/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import path from 'path';
import {
    Type,
    TSFunction,
    TSClass,
    builtinTypes,
    FunctionKind,
    getMethodPrefix,
} from './type.js';
import { ParserContext } from './frontend.js';
import { parentIsFunctionLike, Stack } from './utils.js';
import { Parameter, Variable } from './variable.js';
import { Statement } from './statement.js';
import { ArgNames, BuiltinNames } from '../lib/builtin/builtinUtil.js';

export enum ScopeKind {
    Scope,
    GlobalScope = 'Global',
    FunctionScope = 'Function',
    BlockScope = 'Block',
    ClassScope = 'Class',
    NamespaceScope = 'Namespace',
}

export class Scope {
    kind = ScopeKind.Scope;
    children: Scope[] = [];
    parent: Scope | null;
    namedTypeMap: Map<string, Type> = new Map();
    /* Hold all temp variables inserted during code generation */
    private tempVarArray: Variable[] = [];
    private variableArray: Variable[] = [];
    private statementArray: Statement[] = [];
    private localIndex = -1;
    public mangledName = '';
    private modifiers: ts.Node[] = [];

    addStatement(statement: Statement) {
        this.statementArray.push(statement);
    }

    addStatementAtStart(statement: Statement) {
        this.statementArray.unshift(statement);
    }

    get statements(): Statement[] {
        return this.statementArray;
    }

    constructor(parent: Scope | null) {
        this.parent = parent;
        if (this.parent !== null) {
            this.parent.addChild(this);
        }
    }

    addVariable(variableObj: Variable) {
        this.variableArray.push(variableObj);
        variableObj.scope = this;
    }

    addType(name: string, type: Type) {
        this.namedTypeMap.set(name, type);
    }

    allocateLocalIndex() {
        return this.localIndex++;
    }

    assignVariableIndex(scope: Scope) {
        if (scope instanceof FunctionScope || scope instanceof BlockScope) {
            scope.varArray.forEach((v) => {
                v.setVarIndex(this.localIndex++);
            });
        }

        scope.children.forEach((s) => {
            if (s instanceof BlockScope) {
                this.assignVariableIndex(s);
            }
        });
    }

    initVariableIndex() {
        if (this.localIndex !== -1) {
            throw Error(`Can't initialize variables multiple times`);
        }

        if (this instanceof FunctionScope) {
            this.localIndex = this.paramArray.length;
        } else if (this instanceof GlobalScope) {
            this.localIndex = 0;
        } else {
            return;
        }
        this.assignVariableIndex(this);
    }

    addTempVar(variableObj: Variable) {
        if (this.localIndex === -1) {
            throw Error(
                `Can't add temp variable begore index assigned, add variable instead`,
            );
        }
        this.tempVarArray.push(variableObj);
    }

    getTempVars() {
        return this.tempVarArray;
    }

    get varArray(): Variable[] {
        return this.variableArray;
    }

    addChild(child: Scope) {
        this.children.push(child);
    }

    addModifier(modifier: ts.Node) {
        this.modifiers.push(modifier);
    }

    protected _nestFindScopeItem<T>(
        name: string,
        searchFunc: (scope: Scope) => T | undefined,
        nested = true,
    ): T | undefined {
        let result = searchFunc(this);
        if (result) {
            return result;
        }

        if (nested) {
            result = this.parent?._nestFindScopeItem(name, searchFunc, nested);
        }

        return result;
    }

    findVariable(variableName: string, nested = true): Variable | undefined {
        return this._nestFindScopeItem(
            variableName,
            (scope) => {
                // TODO: process "this" pointer
                let res = scope.variableArray.find((v) => {
                    return v.varName === variableName;
                });

                if (!res && scope instanceof FunctionScope) {
                    res = scope.paramArray.find((v) => {
                        return v.varName === variableName;
                    });
                }

                return res;
            },
            nested,
        );
    }

    findFunctionScope(
        functionName: string,
        nested = true,
    ): FunctionScope | undefined {
        return this._nestFindScopeItem(
            functionName,
            (scope) => {
                return scope.children.find((c) => {
                    return (
                        c.kind === ScopeKind.FunctionScope &&
                        (c as FunctionScope).funcName === functionName
                    );
                }) as FunctionScope;
            },
            nested,
        );
    }

    findNamespaceScope(
        name: string,
        nested = true,
    ): NamespaceScope | undefined {
        return this._nestFindScopeItem(
            name,
            (scope) => {
                return scope.children.find((s) => {
                    return (
                        s.kind === ScopeKind.NamespaceScope &&
                        (<NamespaceScope>s).namespaceName === name
                    );
                }) as NamespaceScope;
            },
            nested,
        );
    }

    findType(typeName: string, nested = true): Type | undefined {
        const res = builtinTypes.get(typeName);
        if (res) {
            return res;
        }

        return this._nestFindScopeItem(
            typeName,
            (scope) => {
                return scope.namedTypeMap.get(typeName);
            },
            nested,
        );
    }

    findIdentifier(
        name: string,
        nested = true,
    ): Variable | Scope | Type | undefined {
        return this._nestFindScopeItem(
            name,
            (scope) => {
                let res: Variable | Scope | Type | undefined;

                /* Step1: Find variable in current scope */
                res =
                    scope.findVariable(name, false) ||
                    /* Step2: Find function in current scope */
                    scope.findFunctionScope(name, false) ||
                    /* Step3: Find type in current scope */
                    scope.findType(name, false) ||
                    /* Step4: Find namespace */
                    scope.findNamespaceScope(name, false);
                if (res) {
                    return res;
                }

                /* Step5: Find in other module*/
                if (scope instanceof GlobalScope) {
                    let searchName: string;
                    // judge if name is default name
                    if (scope.defaultModuleImportMap.has(name)) {
                        const defaultModule =
                            scope.defaultModuleImportMap.get(name)!;
                        searchName = defaultModule.defaultNoun;
                        res = defaultModule.findIdentifier(searchName);
                    } else {
                        if (
                            scope.identifierModuleImportMap.has(name) ||
                            scope.nameAliasImportMap.has(name)
                        ) {
                            const originName =
                                scope.nameAliasImportMap.get(name);
                            searchName = originName ? originName : name;
                            const targetModuleScope =
                                scope.identifierModuleImportMap.get(searchName);
                            if (targetModuleScope) {
                                const targetName =
                                    targetModuleScope.nameAliasExportMap.get(
                                        searchName,
                                    );
                                const oriTargetName = targetName
                                    ? targetName
                                    : searchName;
                                res =
                                    targetModuleScope.findIdentifier(
                                        oriTargetName,
                                    );
                            }
                        } else {
                            // import * as T from xx
                            searchName = name;
                            res =
                                scope.nameScopeModuleImportMap.get(searchName);
                        }
                    }
                }

                return res;
            },
            nested,
        );
    }

    _getScopeByType<T>(type: ScopeKind): T | null {
        let currentScope: Scope | null = this;
        while (currentScope !== null) {
            if (currentScope.kind === type) {
                return <T>currentScope;
            }
            currentScope = currentScope.parent;
        }
        return null;
    }

    getNearestFunctionScope() {
        return this._getScopeByType<FunctionScope>(ScopeKind.FunctionScope);
    }

    getRootGloablScope() {
        return this._getScopeByType<GlobalScope>(ScopeKind.GlobalScope);
    }

    public isDeclare(): boolean {
        let res = false;
        if (
            this.modifiers.find((modifier) => {
                return modifier.kind === ts.SyntaxKind.DeclareKeyword;
            })
        ) {
            res = true;
            return res;
        }
        return this.parent?.isDeclare() || false;
    }

    public isDefault(): boolean {
        return this.modifiers.find((modifier) => {
            return modifier.kind === ts.SyntaxKind.DefaultKeyword;
        }) === undefined
            ? false
            : true;
    }

    public isExport(): boolean {
        return this.modifiers.find((modifier) => {
            return modifier.kind === ts.SyntaxKind.ExportKeyword;
        }) === undefined
            ? false
            : true;
    }

    public isStatic(): boolean {
        return this.modifiers.find((modifier) => {
            return modifier.kind === ts.SyntaxKind.StaticKeyword;
        }) === undefined
            ? false
            : true;
    }

    hasDecorator(name: string): boolean {
        return this.modifiers.find((modifier) => {
            return (
                modifier.kind === ts.SyntaxKind.Decorator &&
                (<ts.Decorator>modifier).expression.getText() === name
            );
        }) === undefined
            ? false
            : true;
    }

    traverseScopTree(traverseMethod: (scope: Scope) => void) {
        traverseMethod(this);
        for (const child of this.children) {
            child.traverseScopTree(traverseMethod);
        }
    }
}

export class ClosureEnvironment extends Scope {
    hasFreeVar = false;
    contextVariable: Variable | null = null;

    constructor(parent: Scope | null = null) {
        super(parent);
        if (
            this instanceof FunctionScope ||
            parent?.getNearestFunctionScope()
        ) {
            /* Add context variable if this scope is inside a function scope */
            const contextVar = new Variable(
                '@context',
                new Type(),
                [],
                -1,
                true,
            );
            this.addVariable(contextVar);
            this.contextVariable = contextVar;
        }
    }
}

export class GlobalScope extends Scope {
    kind = ScopeKind.GlobalScope;
    private _moduleName = '';
    private functionName = '';
    private startFunctionVariableArray: Variable[] = [];
    private functionType = new TSFunction();
    // import {xx} from yy, import zz from yy; store [xx, zz]
    identifierModuleImportMap = new Map<string, GlobalScope>();
    // import * as T from yy, Scope is T
    nameScopeModuleImportMap = new Map<string, GlobalScope>();
    // import {xx as zz} from yy, Alias is zz, store <zz, xx>
    nameAliasImportMap = new Map<string, string>();
    // export alias, export { c as renamed_c }; store <renamed_c, c>
    nameAliasExportMap = new Map<string, string>();
    // default identifier map: import theDefault from "./export-case1"; import theOtherDefault from "./export-case2";
    defaultModuleImportMap = new Map<string, GlobalScope>();
    defaultNoun = '';

    isMarkStart = false;

    constructor(parent: Scope | null = null) {
        super(parent);
    }

    addVariable(variableObj: Variable) {
        super.addVariable(variableObj);
        variableObj.setIsLocalVar(false);
    }

    set moduleName(moduleName: string) {
        this._moduleName = moduleName;
    }

    get moduleName(): string {
        return this._moduleName;
    }

    get startFuncName(): string {
        return this.functionName;
    }

    set startFuncName(name: string) {
        this.functionName = name;
    }

    get startFuncType(): TSFunction {
        return this.functionType;
    }

    addImportIdentifier(identifier: string, moduleScope: GlobalScope) {
        this.identifierModuleImportMap.set(identifier, moduleScope);
    }

    addImportDefaultName(defaultImportName: string, moduleScope: GlobalScope) {
        this.defaultModuleImportMap.set(defaultImportName, moduleScope);
    }

    addImportNameScope(nameScope: string, moduleScope: GlobalScope) {
        this.nameScopeModuleImportMap.set(nameScope, moduleScope);
    }

    setImportNameAlias(nameAliasImportMap: Map<string, string>) {
        for (const [key, value] of nameAliasImportMap) {
            this.nameAliasImportMap.set(key, value);
        }
    }

    setExportNameAlias(nameAliasExportMap: Map<string, string>) {
        for (const [key, value] of nameAliasExportMap) {
            this.nameAliasExportMap.set(key, value);
        }
    }
}

export class FunctionScope extends ClosureEnvironment {
    kind = ScopeKind.FunctionScope;
    private functionName = '';
    private parameterArray: Parameter[] = [];
    private functionType = new TSFunction();
    /* iff the function is a member function, which class it belong to */
    private _classNmae = '';

    constructor(parent: Scope) {
        super(parent);
    }

    getThisIndex() {
        return this.varArray.find((v) => {
            return v.varName === 'this';
        })!.varIndex;
    }

    addParameter(parameter: Parameter) {
        this.parameterArray.push(parameter);
        parameter.scope = this;
    }

    get paramArray(): Parameter[] {
        return this.parameterArray;
    }

    setFuncName(name: string) {
        this.functionName = name;
    }

    get funcName(): string {
        return this.functionName;
    }

    setFuncType(type: TSFunction) {
        this.functionType = type;
    }

    get funcType(): TSFunction {
        return this.functionType;
    }

    setClassName(name: string) {
        this._classNmae = name;
    }

    get className(): string {
        return this._classNmae;
    }

    isMethod(): boolean {
        return this._classNmae !== '';
    }
}

export class BlockScope extends ClosureEnvironment {
    kind = ScopeKind.BlockScope;
    name = '';
    /* belong function scope of this block scope,
        may be null if this block is in global scope */
    funcScope: FunctionScope | null = null;

    constructor(
        parent: Scope,
        name = '',
        funcScope: FunctionScope | null = null,
    ) {
        super(parent);
        this.name = name;
        this.funcScope = funcScope;
    }
}

export class ClassScope extends Scope {
    kind = ScopeKind.ClassScope;
    private _classType: TSClass = new TSClass();
    private _className = '';

    constructor(parent: Scope, name = '') {
        super(parent);
        this._className = name;
    }

    get className(): string {
        return this._className;
    }

    setClassType(type: TSClass) {
        this._classType = type;
    }

    get classType(): TSClass {
        return this._classType;
    }
}

export class NamespaceScope extends Scope {
    kind = ScopeKind.NamespaceScope;
    namespaceName;

    constructor(parent: Scope, name = '') {
        super(parent);
        this.namespaceName = name;
    }

    addVariable(variableObj: Variable) {
        super.addVariable(variableObj);
        variableObj.setIsLocalVar(false);
    }
}

export class ScopeScanner {
    globalScopeStack: Stack<GlobalScope>;
    currentScope: Scope | null = null;
    nodeScopeMap: Map<ts.Node, Scope>;
    /* anonymous function index */
    anonymousIndex = 0;

    constructor(private parserCtx: ParserContext) {
        this.globalScopeStack = this.parserCtx.globalScopeStack;
        this.nodeScopeMap = this.parserCtx.nodeScopeMap;
    }

    _generateClassFuncScope(
        node:
            | ts.AccessorDeclaration
            | ts.MethodDeclaration
            | ts.ConstructorDeclaration,
        methodType: FunctionKind,
    ) {
        const parentScope = this.currentScope!;
        const functionScope = new FunctionScope(parentScope);
        if (node.modifiers !== undefined) {
            for (const modifier of node.modifiers) {
                functionScope.addModifier(modifier);
            }
        }

        functionScope.addParameter(
            new Parameter('@context', new Type(), [], 0, false, false),
        );

        if (methodType !== FunctionKind.STATIC) {
            functionScope.addParameter(
                new Parameter('@this', new Type(), [], 1, false, false),
            );
            functionScope.addVariable(new Variable('this', new Type(), [], -1));
        }

        functionScope.setClassName((<ClassScope>parentScope).className);
        let methodName = getMethodPrefix(methodType);
        if (node.name) {
            methodName += node.name.getText();
        }

        functionScope.setFuncName(methodName);
        this.nodeScopeMap.set(node, functionScope);
        if (!functionScope.isDeclare()) {
            this.setCurrentScope(functionScope);
            this.visitNode(node.body!);
            this.setCurrentScope(parentScope);
        }
    }

    visit(nodes: Array<ts.SourceFile>) {
        for (const sourceFile of nodes) {
            this.visitNode(sourceFile);
        }
    }

    visitNode(node: ts.Node): void {
        switch (node.kind) {
            case ts.SyntaxKind.SourceFile: {
                const sourceFileNode = <ts.SourceFile>node;
                const globalScope = new GlobalScope();
                this.setCurrentScope(globalScope);
                const filePath = sourceFileNode.fileName.slice(
                    undefined,
                    -'.ts'.length,
                );
                const moduleName = path.relative(process.cwd(), filePath);
                if (!this.parserCtx.compileArgs[ArgNames.isBuiltIn]) {
                    globalScope.moduleName = moduleName;
                } else {
                    globalScope.moduleName = 'builtIn';
                }
                this.globalScopeStack.push(globalScope);
                this.nodeScopeMap.set(sourceFileNode, globalScope);
                for (let i = 0; i < sourceFileNode.statements.length; i++) {
                    this.visitNode(sourceFileNode.statements[i]);
                }
                this.visitNode(sourceFileNode.endOfFileToken);
                break;
            }
            case ts.SyntaxKind.ModuleDeclaration: {
                const moduleDeclaration = <ts.ModuleDeclaration>node;
                const namespaceName = moduleDeclaration.name.text;
                const parentScope = this.currentScope!;
                if (
                    parentScope.kind !== ScopeKind.GlobalScope &&
                    parentScope.kind !== ScopeKind.NamespaceScope
                ) {
                    throw Error(
                        'A namespace declaration is only allowed at the top level of a namespace or module',
                    );
                }
                const namespaceScope = new NamespaceScope(
                    parentScope,
                    namespaceName,
                );
                if (moduleDeclaration.modifiers !== undefined) {
                    for (const modifier of moduleDeclaration.modifiers) {
                        namespaceScope.addModifier(modifier);
                    }
                }
                const moduleBlock = <ts.ModuleBlock>moduleDeclaration.body!;
                this.setCurrentScope(namespaceScope);
                this.nodeScopeMap.set(moduleBlock, namespaceScope);
                const statements = moduleBlock.statements;
                if (statements.length !== 0) {
                    for (let i = 0; i < statements.length; i++) {
                        this.visitNode(statements[i]);
                    }
                }
                this.setCurrentScope(parentScope);
                break;
            }
            case ts.SyntaxKind.FunctionDeclaration: {
                const funcDecl = <ts.FunctionDeclaration>node;
                this._generateFuncScope(funcDecl);
                break;
            }
            case ts.SyntaxKind.FunctionExpression: {
                const funcExpr = <ts.FunctionExpression>node;
                this._generateFuncScope(funcExpr);
                break;
            }
            case ts.SyntaxKind.ArrowFunction: {
                const arrowFunc = <ts.ArrowFunction>node;
                this._generateFuncScope(arrowFunc);
                break;
            }
            case ts.SyntaxKind.ClassDeclaration: {
                const classDeclarationNode = <ts.ClassDeclaration>node;
                const parentScope = this.currentScope!;
                const className = (<ts.Identifier>(
                    classDeclarationNode.name
                )).getText();
                const classScope = new ClassScope(parentScope, className);
                if (classDeclarationNode.modifiers) {
                    for (const modifier of classDeclarationNode.modifiers) {
                        classScope.addModifier(modifier);
                    }
                }
                this.setCurrentScope(classScope);
                this.nodeScopeMap.set(classDeclarationNode, classScope);
                for (const member of classDeclarationNode.members) {
                    if (
                        member.kind === ts.SyntaxKind.SetAccessor ||
                        member.kind === ts.SyntaxKind.GetAccessor ||
                        member.kind === ts.SyntaxKind.Constructor ||
                        member.kind === ts.SyntaxKind.MethodDeclaration
                    ) {
                        this.visitNode(member);
                    }
                }
                this.setCurrentScope(parentScope);
                break;
            }
            case ts.SyntaxKind.SetAccessor: {
                if ((<ts.SetAccessorDeclaration>node).body) {
                    this._generateClassFuncScope(
                        <ts.MethodDeclaration>node,
                        FunctionKind.SETTER,
                    );
                }
                break;
            }
            case ts.SyntaxKind.GetAccessor: {
                if ((<ts.GetAccessorDeclaration>node).body) {
                    this._generateClassFuncScope(
                        <ts.MethodDeclaration>node,
                        FunctionKind.GETTER,
                    );
                }
                break;
            }
            case ts.SyntaxKind.Constructor: {
                this._generateClassFuncScope(
                    <ts.ConstructorDeclaration>node,
                    FunctionKind.CONSTRUCTOR,
                );
                break;
            }
            case ts.SyntaxKind.MethodDeclaration: {
                const methodNode = <ts.MethodDeclaration>node;
                const kind = methodNode.modifiers?.find((m) => {
                    return m.kind === ts.SyntaxKind.StaticKeyword;
                })
                    ? FunctionKind.STATIC
                    : FunctionKind.METHOD;
                this._generateClassFuncScope(methodNode, kind);
                break;
            }
            case ts.SyntaxKind.Block: {
                const blockNode = <ts.Block>node;
                this.createBlockScope(blockNode);
                break;
            }
            case ts.SyntaxKind.ForStatement: {
                const forStatementNode = <ts.ForStatement>node;
                this.createLoopBlockScope(forStatementNode);
                break;
            }
            case ts.SyntaxKind.WhileStatement: {
                const whileStatementNode = <ts.WhileStatement>node;
                this.createLoopBlockScope(whileStatementNode);
                break;
            }
            case ts.SyntaxKind.DoStatement: {
                const doStatementNode = <ts.DoStatement>node;
                this.createLoopBlockScope(doStatementNode);
                break;
            }
            case ts.SyntaxKind.CaseClause: {
                const caseClauseNode = <ts.CaseClause>node;
                this.createBlockScope(caseClauseNode);
                break;
            }
            case ts.SyntaxKind.DefaultClause: {
                const defaultClauseNode = <ts.DefaultClause>node;
                this.createBlockScope(defaultClauseNode);
                break;
            }
            default: {
                ts.forEachChild(node, this.visitNode.bind(this));
            }
        }
    }

    setCurrentScope(currentScope: Scope | null) {
        this.currentScope = currentScope;
    }

    createBlockScope(node: ts.BlockLike) {
        const parentScope = this.currentScope!;

        if (!parentIsFunctionLike(node)) {
            const parentScope = this.currentScope!;
            const parentName = ScopeScanner.getPossibleScopeName(parentScope);
            const blockName = ts.isCaseOrDefaultClause(node) ? 'case' : 'block';
            const maybeFuncScope = parentScope.getNearestFunctionScope();
            const blockScope = new BlockScope(
                parentScope,
                `${parentName}.${blockName}`,
                maybeFuncScope,
            );
            this.setCurrentScope(blockScope);
            this.nodeScopeMap.set(node, blockScope);
        }

        const statements = node.statements;
        if (statements.length !== 0) {
            for (let i = 0; i < statements.length; i++) {
                this.visitNode(statements[i]);
            }
        }

        if (!parentIsFunctionLike(node)) {
            this.setCurrentScope(parentScope);
        }
    }

    createLoopBlockScope(
        node: ts.ForStatement | ts.WhileStatement | ts.DoStatement,
    ) {
        const parentScope = this.currentScope!;
        const parentName = ScopeScanner.getPossibleScopeName(parentScope);
        const maybeFuncScope = parentScope.getNearestFunctionScope();
        const outOfLoopBlock = new BlockScope(
            parentScope,
            `${parentName}.loop`,
            maybeFuncScope,
        );
        this.setCurrentScope(outOfLoopBlock);
        this.nodeScopeMap.set(node, outOfLoopBlock);

        this.visitNode(node.statement);

        this.setCurrentScope(parentScope);
    }

    private _generateFuncScope(
        node: ts.FunctionDeclaration | ts.FunctionExpression | ts.ArrowFunction,
    ) {
        const parentScope = this.currentScope!;
        const functionScope = new FunctionScope(parentScope);
        /* function context struct placeholder */
        functionScope.addParameter(
            new Parameter('@context', new Type(), [], 0, false, false),
        );
        if (node.modifiers !== undefined) {
            for (const modifier of node.modifiers) {
                functionScope.addModifier(modifier);
            }
        }
        let functionName: string;
        if (node.name !== undefined) {
            functionName = node.name.getText();
        } else {
            functionName = 'anonymous' + this.anonymousIndex++;
        }

        functionScope.setFuncName(functionName);
        this.nodeScopeMap.set(node, functionScope);

        if (functionScope.isDefault()) {
            functionScope.getRootGloablScope()!.defaultNoun = functionName;
        }
        if (!functionScope.isDeclare()) {
            this.setCurrentScope(functionScope);
            this.visitNode(node.body!);
            this.setCurrentScope(parentScope);
        }
    }

    static getPossibleScopeName(scope: Scope) {
        let possibleName = '';
        if (scope.kind === ScopeKind.FunctionScope) {
            possibleName = (scope as FunctionScope).funcName;
        } else if (scope.kind === ScopeKind.GlobalScope) {
            possibleName = 'global';
        } else if (scope.kind === ScopeKind.ClassScope) {
            possibleName = (scope as ClassScope).className;
        } else {
            possibleName = (scope as BlockScope).name;
        }

        return possibleName;
    }
}
