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
import SematicCheck from './semantic_check.js';
import { ArgNames, BuiltinNames } from '../lib/builtin/builtin_name.js';

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
    noLib: true,
};

export class ParserContext {
    private _scopeScanner;
    private _typeResolver;
    private _variableScanner;
    private _variableInit;
    private _exprProcessor;
    private _stmtProcessor;
    private _sematicChecker;
    private _errorMessage: ts.Diagnostic[] | null = null;

    typeChecker: ts.TypeChecker | undefined;
    globalScopes = new Array<GlobalScope>();
    nodeScopeMap = new Map<ts.Node, Scope>();
    currentScope: Scope | null = null;

    typeId = 0;
    /* mapping type_string to type id */
    typeIdMap = new Map<string, number>();

    // configurations
    compileArgs: CompileArgs = {};
    builtInPath = path.join(
        path.dirname(fileURLToPath(import.meta.url)),
        '..',
        'lib',
        'builtin',
    );
    builtInFileNames = BuiltinNames.builtinFileNames.map((builtInFileName) => {
        return path.join(this.builtInPath, builtInFileName);
    });

    constructor() {
        this._scopeScanner = new ScopeScanner(this);
        this._typeResolver = new TypeResolver(this);
        this._sematicChecker = new SematicCheck();
        this._variableScanner = new VariableScanner(this);
        this._variableInit = new VariableInit(this);
        this._exprProcessor = new ExpressionCompiler(this);
        this._stmtProcessor = new StatementCompiler(this);
    }

    parse(fileNames: string[], compileArgs: CompileArgs = {}): void {
        this.compileArgs = compileArgs;
        const compilerOptions: ts.CompilerOptions = this.getCompilerOptions();
        let rootNames = [...fileNames];
        if (!compileArgs[ArgNames.disableBuiltIn]) {
            rootNames = [...this.builtInFileNames, ...fileNames];
        }
        const program: ts.Program = ts.createProgram(
            rootNames,
            compilerOptions,
        );
        this.typeChecker = program.getTypeChecker();

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

            this._errorMessage = allDiagnostics as ts.Diagnostic[];
            throw new SyntaxError(formattedError);
        }

        const sourceFileList = Array.from(program.getSourceFiles());

        /* Step1: Resolve all scopes */
        this._scopeScanner.visit(sourceFileList);
        /* Step2: Resolve all type declarations */
        this._typeResolver.visit();
        /* User scope must import the standard library module manually */
        if (!compileArgs[ArgNames.disableBuiltIn]) {
            const builtInScope = this.globalScopes[1];
            for (
                let i = this.builtInFileNames.length;
                i < this.globalScopes.length;
                i++
            ) {
                for (const builtInIdentifier of BuiltinNames.builtinIdentifierArray) {
                    this.globalScopes[i].addImportIdentifier(
                        builtInIdentifier,
                        builtInScope,
                    );
                }
            }
        }
        /* Step3: Add variables to scopes */
        this._variableScanner.visit();
        this._variableInit.visit();
        /* Step4: Mangling function and global variable name */
        mangling(this.globalScopes);
        /* Step5: Add statements to scopes */
        this._stmtProcessor.visit();
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

    get typeResolver() {
        return this._typeResolver;
    }

    get expressionCompiler(): ExpressionCompiler {
        return this._exprProcessor;
    }

    get statementCompiler(): StatementCompiler {
        return this._stmtProcessor;
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

        for (let i = 0; i < this.globalScopes.length; ++i) {
            const scope = this.globalScopes[i];
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
