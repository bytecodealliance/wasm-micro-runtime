/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import TypeResolver from './type.js';
import { mangling, Stack } from './utils.js';
import { fileURLToPath } from 'url';
import {
    FunctionScope,
    GlobalScope,
    Scope,
    ScopeKind,
    ScopeScanner,
} from './scope.js';
import { VariableScanner, VariableInit } from './variable.js';
import ExpressionCompiler from './expression.js';
import StatementCompiler from './statement.js';
import path from 'path';
import { Logger } from './log.js';
import { SyntaxError } from './error.js';
import SematicCheck from './sematicCheck.js';

export interface CompileArgs {
    [key: string]: any;
}

export const COMPILER_OPTIONS: ts.CompilerOptions = {
    module: ts.ModuleKind.ESNext,
    target: ts.ScriptTarget.ES2015,
    strict: true,
    /* disable some features to speedup tsc */
    skipLibCheck: true,
    skipDefaultLibCheck: true,
    types: [],
    experimentalDecorators: true,
};

export class ParserContext {
    private scopeScanner;
    private typeCompiler;
    private _sematicChecker;
    private variableScanner;
    private variableInit;
    private exprCompiler;
    private stmtCompiler;
    private _errorMessage: ts.Diagnostic[] | null = null;

    typeChecker: ts.TypeChecker | undefined;
    globalScopeStack = new Stack<GlobalScope>();
    nodeScopeMap = new Map<ts.Node, Scope>();
    currentScope: Scope | null = null;

    typeId = 0;
    /* mapping type_string to type id */
    typeIdMap = new Map<string, number>();

    // configurations
    compileArgs: CompileArgs = {};

    constructor() {
        this.scopeScanner = new ScopeScanner(this);
        this.typeCompiler = new TypeResolver(this);
        this._sematicChecker = new SematicCheck();
        this.variableScanner = new VariableScanner(this);
        this.variableInit = new VariableInit(this);
        this.exprCompiler = new ExpressionCompiler(this);
        this.stmtCompiler = new StatementCompiler(this);
    }

    parse(fileNames: string[], compileArgs: CompileArgs = {}): void {
        const compilerOptions: ts.CompilerOptions = this.getCompilerOptions();
        const program: ts.Program = ts.createProgram(
            fileNames,
            compilerOptions,
        );
        this.typeChecker = program.getTypeChecker();
        this.compileArgs = compileArgs;

        const allDiagnostics = ts.getPreEmitDiagnostics(program);
        if (allDiagnostics.length > 0) {
            const formattedError = ts.formatDiagnosticsWithColorAndContext(
                allDiagnostics,
                {
                    getCurrentDirectory: () => {
                        return path.dirname(fileURLToPath(import.meta.url));
                    },
                    getCanonicalFileName: (fileNames) => {
                        return fileNames;
                    },
                    getNewLine: () => {
                        return '\n';
                    },
                },
            );
            console.log(formattedError);
            this._errorMessage = allDiagnostics as ts.Diagnostic[];
            throw new SyntaxError('Syntax error in source file.');
        }

        const sourceFileList = program
            .getSourceFiles()
            .filter(
                (sourceFile: ts.SourceFile) =>
                    !sourceFile.fileName.match(/\.d\.ts$/),
            );

        /* Step1: Resolve all scopes */
        this.scopeScanner.visit(sourceFileList);
        /* Step2: Resolve all type declarations */
        this.typeCompiler.visit();
        /* Step3: Add variables to scopes */
        this.variableScanner.visit();
        this.variableInit.visit();
        /* Step4: Mangling function and global variable name */
        const globalScopeArray = [];
        for (let i = 0; i < this.globalScopeStack.size(); i++) {
            globalScopeArray.push(this.globalScopeStack.getItemAtIdx(i));
        }
        mangling(globalScopeArray);
        /* Step5: Add statements to scopes */
        this.stmtCompiler.visit();
        /* Step6: Additional semantic check */
        this.sematicChecker.checkRes();

        this.recordScopes();
        if (process.env['TS2WASM_DUMP_SCOPE']) {
            this.dumpScopes();
        }
    }

    getScopeByNode(node: ts.Node): Scope | undefined {
        let res: Scope | undefined;

        while (node) {
            res = this.nodeScopeMap.get(node);
            if (res) {
                break;
            }
            node = node.parent;
        }

        return res;
    }

    get typeComp() {
        return this.typeCompiler;
    }

    get expressionCompiler(): ExpressionCompiler {
        return this.exprCompiler;
    }

    get statementCompiler(): StatementCompiler {
        return this.stmtCompiler;
    }

    get sematicChecker(): SematicCheck {
        return this._sematicChecker;
    }

    get errorMessage() {
        return this._errorMessage;
    }

    recordScopes() {
        const scopes = this.generateScopes();
        const scopeInfos: Array<any> = scopes.scopeInfos;
        const scopeVarInfos: Array<any> = scopes.scopeVarInfos;
        const scopeTypeInfos: Array<any> = scopes.scopeTypeInfos;
        for (let i = 0; i < scopeInfos.length; ++i) {
            Logger.debug(
                `============= Variables in scope '${scopeInfos[i].name}' (${scopeInfos[i].kind}) =============`,
            );
            Logger.debug(scopeVarInfos[i]);
            Logger.debug(
                `============= Types in scope '${scopeInfos[i].name}' (${scopeInfos[i].kind})=============`,
            );
            Logger.debug(scopeTypeInfos[i]);
        }
        Logger.debug(`============= Scope Summary =============`);
        Logger.debug(scopeInfos);
    }

    dumpScopes() {
        const scopes = this.generateScopes();
        const scopeInfos: Array<any> = scopes.scopeInfos;
        const scopeVarInfos: Array<any> = scopes.scopeVarInfos;
        const scopeTypeInfos: Array<any> = scopes.scopeTypeInfos;
        for (let i = 0; i < scopeInfos.length; ++i) {
            console.log(
                `============= Variables in scope '${scopeInfos[i].name}' (${scopeInfos[i].kind}) =============`,
            );
            console.table(scopeVarInfos[i]);
            console.log(
                `============= Types in scope '${scopeInfos[i].name}' (${scopeInfos[i].kind})=============`,
            );
            console.table(scopeTypeInfos[i]);
        }
        console.log(`============= Scope Summary =============`);
        console.table(scopeInfos);
    }

    generateScopes() {
        const scopeInfos: Array<any> = [];
        const scopeVarInfos: Array<any> = [];
        const scopeTypeInfos: Array<any> = [];

        for (let i = 0; i < this.globalScopeStack.size(); ++i) {
            const scope = this.globalScopeStack.getItemAtIdx(i);
            scope.traverseScopTree((scope) => {
                const scopeName = ScopeScanner.getPossibleScopeName(scope);
                let paramCount = 0;

                if (scope.kind === ScopeKind.FunctionScope) {
                    const funcScope = <FunctionScope>scope;
                    paramCount = funcScope.paramArray.length;
                }

                scopeInfos.push({
                    kind: `${scope.kind}`,
                    name: scopeName,
                    param_cnt: paramCount,
                    var_cnt: scope.varArray.length,
                    stmt_cnt: scope.statements.length,
                    child_cnt: scope.children.length,
                });

                const varInfos: Array<any> = [];
                if (scope.kind === ScopeKind.FunctionScope) {
                    (<FunctionScope>scope).paramArray.forEach((v) => {
                        let displayName = v.varName;
                        if (displayName === '') {
                            displayName = '@context';
                        }
                        varInfos.push({
                            kind: 'param',
                            name: displayName,
                            type: v.varType,
                            isClosure: v.varIsClosure,
                            modifiers: v.varModifiers,
                            index: v.varIndex,
                        });
                    });
                }

                scope.varArray.forEach((v) => {
                    let displayName = v.varName;
                    if (displayName === '') {
                        displayName = '@context';
                    }
                    varInfos.push({
                        kind: 'var',
                        name: displayName,
                        type: v.varType,
                        isClosure: v.varIsClosure,
                        modifiers: v.varModifiers,
                        index: v.varIndex,
                    });
                });
                scopeVarInfos.push(varInfos);

                const typeInfos: Array<any> = [];
                scope.namedTypeMap.forEach((t, name) => {
                    typeInfos.push({
                        name: name,
                        type: t,
                    });
                });
                scopeTypeInfos.push(typeInfos);
            });
        }
        return { scopeInfos, scopeVarInfos, scopeTypeInfos };
    }

    private getCompilerOptions() {
        const opts: ts.CompilerOptions = {};
        for (const i of Object.keys(COMPILER_OPTIONS)) {
            opts[i] = COMPILER_OPTIONS[i];
        }
        return opts;
    }
}
