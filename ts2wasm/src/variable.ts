/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import { Expression } from './expression.js';
import { FunctionKind, TSFunction, Type } from './type.js';
import { ParserContext } from './frontend.js';
import { generateNodeExpression, isScopeNode } from './utils.js';
import { ClassScope, FunctionScope, GlobalScope, Scope } from './scope.js';

export enum ModifierKind {
    default = '',
    const = 'const',
    let = 'let',
    var = 'var',
}
export class Variable {
    private _isClosure = false;
    private _closureIndex = 0;
    public mangledName = '';
    public scope: Scope | null = null;

    constructor(
        private name: string,
        private type: Type,
        private modifiers: (ModifierKind | ts.SyntaxKind)[] = [],
        private index = -1,
        private isLocal = true,
        private init: Expression | null = null,
    ) {}

    get varName(): string {
        return this.name;
    }

    set varType(type: Type) {
        this.type = type;
    }

    get varType(): Type {
        return this.type;
    }

    get varModifiers(): (ModifierKind | ts.SyntaxKind)[] {
        return this.modifiers;
    }

    public isConst(): boolean {
        return this.modifiers.includes(ModifierKind.const);
    }

    public isReadOnly(): boolean {
        return this.modifiers.includes(ts.SyntaxKind.ReadonlyKeyword);
    }

    public isDeclare(): boolean {
        let res = false;
        if (this.modifiers.includes(ts.SyntaxKind.DeclareKeyword)) {
            res = true;
            return res;
        }
        return this.scope?.isDeclare() || false;
    }

    public isExport(): boolean {
        return this.modifiers.includes(ts.SyntaxKind.ExportKeyword);
    }

    public isDefault(): boolean {
        return this.modifiers.includes(ts.SyntaxKind.DefaultKeyword);
    }

    public isFuncScopedVar(): boolean {
        return this.modifiers.includes(ModifierKind.var);
    }

    public setInitExpr(expr: Expression) {
        this.init = expr;
    }

    get initExpression(): Expression | null {
        return this.init;
    }

    get varIsClosure(): boolean {
        return this._isClosure;
    }

    public setClosureIndex(index: number) {
        this._closureIndex = index;
    }

    public getClosureIndex(): number {
        return this._closureIndex;
    }

    public setVarIndex(varIndex: number) {
        this.index = varIndex;
    }

    get varIndex(): number {
        return this.index;
    }

    public isLocalVar(): boolean {
        return this.isLocal;
    }

    public setIsLocalVar(isLocal: boolean): void {
        this.isLocal = isLocal;
    }

    public setVarIsClosure(): void {
        this._isClosure = true;
    }
}

export class Parameter extends Variable {
    private _isOptional: boolean;
    private _isDestructuring: boolean;

    constructor(
        name: string,
        type: Type,
        modifiers: (ModifierKind | ts.SyntaxKind)[] = [],
        index = -1,
        isOptional = false,
        isDestructuring = false,
        init: Expression | null = null,
        isLocal = true,
    ) {
        super(name, type, modifiers, index, isLocal, init);
        this._isOptional = isOptional;
        this._isDestructuring = isDestructuring;
    }

    get optional(): boolean {
        return this._isOptional;
    }

    get destructuring(): boolean {
        return this._isDestructuring;
    }
}

export class VariableScanner {
    typechecker: ts.TypeChecker | undefined = undefined;
    globalScopes = new Array<GlobalScope>();
    currentScope: Scope | null = null;
    nodeScopeMap = new Map<ts.Node, Scope>();

    constructor(private parserCtx: ParserContext) {
        this.globalScopes = this.parserCtx.globalScopes;
        this.nodeScopeMap = this.parserCtx.nodeScopeMap;
    }

    visit() {
        this.typechecker = this.parserCtx.typeChecker;
        this.nodeScopeMap.forEach((scope, node) => {
            this.currentScope = scope;
            ts.forEachChild(node, this.visitNode.bind(this));

            if (scope instanceof FunctionScope) {
                const classScope = scope.parent as ClassScope;
                if (scope.funcType.funcKind !== FunctionKind.DEFAULT) {
                    /* For class methods, fix type for "this" parameter */
                    if (!scope.isStatic()) {
                        /**
                         * varArray[0] - context
                         * varArray[0] - this
                         */
                        scope.varArray[1].varType = classScope.classType;
                    }
                }
            }
        });

        for (let i = 0; i < this.globalScopes.length; ++i) {
            const scope = this.globalScopes[i];
            scope.traverseScopTree((scope) => {
                if (
                    scope instanceof FunctionScope ||
                    scope instanceof GlobalScope
                ) {
                    /* Assign index for function variables */
                    scope.initVariableIndex();
                }
            });
        }
    }

    visitNode(node: ts.Node): void {
        switch (node.kind) {
            case ts.SyntaxKind.Parameter: {
                if (
                    node.parent.kind === ts.SyntaxKind.FunctionType ||
                    (node.parent &&
                        node.parent.parent.kind ===
                            ts.SyntaxKind.InterfaceDeclaration)
                ) {
                    break;
                }
                const parameterNode = <ts.ParameterDeclaration>node;
                const functionScope = <FunctionScope>(
                    this.currentScope!.getNearestFunctionScope()
                );
                // TODO: have not record DotDotDotToken
                const paramName = parameterNode.name.getText();
                let isDestructuring = false;
                if (
                    parameterNode.name.kind ===
                    ts.SyntaxKind.ObjectBindingPattern
                ) {
                    isDestructuring = true;
                }
                let isOptional =
                    parameterNode.questionToken === undefined ? false : true;
                isOptional =
                    isOptional || parameterNode.initializer === undefined
                        ? false
                        : true;
                const paramModifiers = [];
                if (parameterNode.modifiers !== undefined) {
                    for (const modifier of parameterNode.modifiers) {
                        paramModifiers.push(modifier.kind);
                    }
                }
                const typeString = this.typechecker!.typeToString(
                    this.typechecker!.getTypeAtLocation(node),
                );
                const paramType = functionScope.findType(typeString);
                const paramIndex = functionScope.paramArray.length;
                const paramObj = new Parameter(
                    paramName,
                    paramType!,
                    paramModifiers,
                    paramIndex,
                    isOptional,
                    isDestructuring,
                );
                functionScope.addParameter(paramObj);
                break;
            }
            case ts.SyntaxKind.VariableDeclaration: {
                const variableDeclarationNode = <ts.VariableDeclaration>node;
                const currentScope = this.currentScope!;

                let variableModifier = ModifierKind.default;
                const variableAssignText =
                    variableDeclarationNode.parent.getText();
                if (variableAssignText.includes(ModifierKind.const)) {
                    variableModifier = ModifierKind.const;
                } else if (variableAssignText.includes(ModifierKind.let)) {
                    variableModifier = ModifierKind.let;
                } else if (variableAssignText.includes(ModifierKind.var)) {
                    variableModifier = ModifierKind.var;
                }
                const varModifiers = [];
                varModifiers.push(variableModifier);
                const stmtNode = variableDeclarationNode.parent.parent;
                if (ts.isVariableStatement(stmtNode) && stmtNode.modifiers) {
                    for (const modifier of stmtNode.modifiers) {
                        varModifiers.push(modifier.kind);
                    }
                }

                const variableName = variableDeclarationNode.name.getText();
                const typeName = this.typechecker!.typeToString(
                    this.typechecker!.getTypeAtLocation(node),
                );
                let variableType = currentScope.findType(typeName);

                /* Sometimes the variable's type is inferred as a TSFunction with
                    isDeclare == true, we need to treat it as non declare function
                    here to keep the same calling convention for closure */
                if (
                    variableType instanceof TSFunction &&
                    variableType.isDeclare
                ) {
                    variableType = variableType.clone();
                    (variableType as TSFunction).isDeclare = false;
                }

                const variable = new Variable(
                    variableName,
                    variableType!,
                    varModifiers,
                    -1,
                    true,
                );
                if (variable.isDefault()) {
                    currentScope.getRootGloablScope()!.defaultNoun =
                        variable.varName;
                }

                /** Variables defined by var can be defined repeatedly */
                const funcScope = currentScope.getNearestFunctionScope();
                let belongScope: Scope;
                if (funcScope) {
                    if (variable.isFuncScopedVar()) {
                        belongScope = funcScope;
                    } else {
                        belongScope = currentScope;
                    }
                } else {
                    belongScope = currentScope;
                }
                const existVar = belongScope.findVariable(variableName, false);
                if (!existVar) {
                    belongScope.addVariable(variable);
                }
                break;
            }
            default: {
                if (isScopeNode(node)) {
                    break;
                }
                ts.forEachChild(node, this.visitNode.bind(this));
            }
        }
    }
}

export class VariableInit {
    typechecker: ts.TypeChecker | undefined = undefined;
    globalScopes = new Array<GlobalScope>();
    currentScope: Scope | null = null;
    nodeScopeMap = new Map<ts.Node, Scope>();

    constructor(private parserCtx: ParserContext) {
        this.globalScopes = this.parserCtx.globalScopes;
        this.nodeScopeMap = this.parserCtx.nodeScopeMap;
    }

    visit() {
        this.typechecker = this.parserCtx.typeChecker;
        this.nodeScopeMap.forEach((scope, node) => {
            this.currentScope = scope;
            ts.forEachChild(node, this.visitNode.bind(this));
        });
    }

    visitNode(node: ts.Node): void {
        switch (node.kind) {
            case ts.SyntaxKind.Parameter: {
                if (
                    node.parent.kind === ts.SyntaxKind.FunctionType ||
                    (node.parent &&
                        node.parent.parent.kind ===
                            ts.SyntaxKind.InterfaceDeclaration)
                ) {
                    break;
                }
                const parameterNode = <ts.ParameterDeclaration>node;
                const functionScope = <FunctionScope>(
                    this.currentScope!.getNearestFunctionScope()
                );
                const paramName = parameterNode.name.getText();
                const paramObj = functionScope.findVariable(paramName);
                if (!paramObj) {
                    throw new Error(
                        "don't find " + paramName + ' in current scope',
                    );
                }
                if (parameterNode.initializer) {
                    const paramInit = generateNodeExpression(
                        this.parserCtx.expressionProcessor,
                        parameterNode.initializer,
                    );
                    paramObj.setInitExpr(paramInit);
                }
                break;
            }
            case ts.SyntaxKind.VariableDeclaration: {
                const variableDeclarationNode = <ts.VariableDeclaration>node;
                const currentScope = this.currentScope!;
                const variableName = variableDeclarationNode.name.getText();
                const variableObj = currentScope.findVariable(variableName);
                if (!variableObj) {
                    throw new Error(
                        "don't find " + variableName + ' in current scope',
                    );
                }
                if (
                    !variableObj.isFuncScopedVar() &&
                    variableDeclarationNode.initializer
                ) {
                    this.parserCtx.currentScope = currentScope;
                    const variableInit = generateNodeExpression(
                        this.parserCtx.expressionProcessor,
                        variableDeclarationNode.initializer,
                    );
                    variableObj.setInitExpr(variableInit);
                }
                break;
            }
            default: {
                if (isScopeNode(node)) {
                    break;
                }
                ts.forEachChild(node, this.visitNode.bind(this));
            }
        }
    }
}
