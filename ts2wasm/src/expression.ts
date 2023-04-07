/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import { ParserContext } from './frontend.js';
import { ClosureEnvironment, FunctionScope } from './scope.js';
import { Variable } from './variable.js';
import { getCurScope } from './utils.js';
import { TSFunction, Type, TypeKind } from './type.js';
import { Logger } from './log.js';

type OperatorKind = ts.SyntaxKind;
type ExpressionKind = ts.SyntaxKind;

export class Expression {
    private kind: ExpressionKind;
    private type: Type = new Type();

    constructor(kind: ExpressionKind) {
        this.kind = kind;
    }

    get expressionKind() {
        return this.kind;
    }

    setExprType(type: Type) {
        this.type = type;
    }

    get exprType(): Type {
        return this.type;
    }
}

export class NullKeywordExpression extends Expression {
    constructor() {
        super(ts.SyntaxKind.NullKeyword);
    }
}

export class NumberLiteralExpression extends Expression {
    private value: number;

    constructor(value: number) {
        super(ts.SyntaxKind.NumericLiteral);
        this.value = value;
    }

    get expressionValue(): number {
        return this.value;
    }
}

export class StringLiteralExpression extends Expression {
    private value: string;

    constructor(value: string) {
        super(ts.SyntaxKind.StringLiteral);
        this.value = value;
    }

    get expressionValue(): string {
        return this.value;
    }
}

export class ObjectLiteralExpression extends Expression {
    constructor(
        private fields: IdentifierExpression[],
        private values: Expression[],
    ) {
        super(ts.SyntaxKind.ObjectLiteralExpression);
    }

    get objectFields(): IdentifierExpression[] {
        return this.fields;
    }

    get objectValues(): Expression[] {
        return this.values;
    }
}

export class ArrayLiteralExpression extends Expression {
    constructor(private elements: Expression[]) {
        super(ts.SyntaxKind.ArrayLiteralExpression);
    }

    get arrayValues(): Expression[] {
        return this.elements;
    }
}

export class FalseLiteralExpression extends Expression {
    constructor() {
        super(ts.SyntaxKind.FalseKeyword);
    }
}

export class TrueLiteralExpression extends Expression {
    constructor() {
        super(ts.SyntaxKind.TrueKeyword);
    }
}

export class IdentifierExpression extends Expression {
    private identifier: string;

    constructor(identifier: string) {
        super(ts.SyntaxKind.Identifier);
        this.identifier = identifier;
    }

    get identifierName(): string {
        return this.identifier;
    }
}

export class BinaryExpression extends Expression {
    private operator: OperatorKind;
    private left: Expression;
    private right: Expression;

    constructor(operator: OperatorKind, left: Expression, right: Expression) {
        super(ts.SyntaxKind.BinaryExpression);
        this.operator = operator;
        this.left = left;
        this.right = right;
    }

    get operatorKind(): OperatorKind {
        return this.operator;
    }

    get leftOperand(): Expression {
        return this.left;
    }

    get rightOperand(): Expression {
        return this.right;
    }
}

export class UnaryExpression extends Expression {
    private operator: OperatorKind;
    private _operand: Expression;

    constructor(
        kind: ExpressionKind,
        operator: OperatorKind,
        operand: Expression,
    ) {
        super(kind);
        this.operator = operator;
        this._operand = operand;
    }

    get operatorKind(): OperatorKind {
        return this.operator;
    }

    get operand(): Expression {
        return this._operand;
    }
}

export class ConditionalExpression extends Expression {
    constructor(
        private cond: Expression,
        private trueExpr: Expression,
        private falseExpr: Expression,
    ) {
        super(ts.SyntaxKind.ConditionalExpression);
    }

    get condtion(): Expression {
        return this.cond;
    }

    get whenTrue(): Expression {
        return this.trueExpr;
    }

    get whenFalse(): Expression {
        return this.falseExpr;
    }
}

export class CallExpression extends Expression {
    private expr: Expression;
    private args: Expression[];

    constructor(
        expr: Expression,
        args: Expression[] = new Array<Expression>(0),
    ) {
        super(ts.SyntaxKind.CallExpression);
        this.expr = expr;
        this.args = args;
    }

    get callExpr(): Expression {
        return this.expr;
    }

    get callArgs(): Expression[] {
        return this.args;
    }
}

export class SuperCallExpression extends Expression {
    private args: Expression[];

    constructor(args: Expression[] = new Array<Expression>(0)) {
        super(ts.SyntaxKind.SuperKeyword);
        this.args = args;
    }

    get callArgs(): Expression[] {
        return this.args;
    }
}

export class PropertyAccessExpression extends Expression {
    private expr: Expression;
    private property: Expression;
    accessSetter = false;

    constructor(expr: Expression, property: Expression) {
        super(ts.SyntaxKind.PropertyAccessExpression);
        this.expr = expr;
        this.property = property;
    }

    get propertyAccessExpr(): Expression {
        return this.expr;
    }

    get propertyExpr(): Expression {
        return this.property;
    }
}

export class NewExpression extends Expression {
    private expr: Expression;
    private arguments: Array<Expression> | undefined;
    private newArrayLen = 0;
    private lenExpression: Expression | null = null;

    constructor(expr: Expression, args?: Array<Expression>) {
        super(ts.SyntaxKind.NewExpression);
        this.expr = expr;
        this.arguments = args;
    }

    get newExpr(): Expression {
        return this.expr;
    }

    setArgs(args: Array<Expression>) {
        this.arguments = args;
    }

    get newArgs(): Array<Expression> | undefined {
        return this.arguments;
    }

    setArrayLen(arrayLen: number) {
        this.newArrayLen = arrayLen;
    }

    get arrayLen(): number {
        return this.newArrayLen;
    }

    setLenExpr(len: Expression) {
        this.lenExpression = len;
    }

    get lenExpr(): Expression | null {
        return this.lenExpression;
    }
}

export class ParenthesizedExpression extends Expression {
    private expr: Expression;

    constructor(expr: Expression) {
        super(ts.SyntaxKind.ParenthesizedExpression);
        this.expr = expr;
    }

    get parentesizedExpr(): Expression {
        return this.expr;
    }
}

export class ElementAccessExpression extends Expression {
    private expr: Expression;
    private argumentExpr: Expression;

    constructor(expr: Expression, argExpr: Expression) {
        super(ts.SyntaxKind.ElementAccessExpression);
        this.expr = expr;
        this.argumentExpr = argExpr;
    }

    get accessExpr(): Expression {
        return this.expr;
    }

    get argExpr(): Expression {
        return this.argumentExpr;
    }
}

export class AsExpression extends Expression {
    private expr: Expression;

    constructor(expr: Expression) {
        super(ts.SyntaxKind.AsExpression);
        this.expr = expr;
    }

    get expression(): Expression {
        return this.expr;
    }
}

export class FunctionExpression extends Expression {
    private _funcScope: FunctionScope;

    constructor(func: FunctionScope) {
        super(ts.SyntaxKind.FunctionExpression);
        this._funcScope = func;
    }

    get funcScope(): FunctionScope {
        return this._funcScope;
    }
}

export default class ExpressionProcessor {
    private typeResolver;
    private nodeScopeMap;

    constructor(private parserCtx: ParserContext) {
        this.typeResolver = this.parserCtx.typeResolver;
        this.nodeScopeMap = this.parserCtx.nodeScopeMap;
    }

    visitNode(node: ts.Node): Expression {
        switch (node.kind) {
            case ts.SyntaxKind.NullKeyword: {
                const nullExpr = new NullKeywordExpression();
                nullExpr.setExprType(this.typeResolver.generateNodeType(node));
                return nullExpr;
            }
            case ts.SyntaxKind.NumericLiteral: {
                const numberLiteralExpr = new NumberLiteralExpression(
                    parseFloat((<ts.NumericLiteral>node).getText()),
                );
                numberLiteralExpr.setExprType(
                    this.typeResolver.generateNodeType(node),
                );
                return numberLiteralExpr;
            }
            case ts.SyntaxKind.StringLiteral: {
                const stringLiteralExpr = new StringLiteralExpression(
                    (<ts.StringLiteral>node).getText(),
                );
                stringLiteralExpr.setExprType(
                    this.typeResolver.generateNodeType(node),
                );
                return stringLiteralExpr;
            }
            case ts.SyntaxKind.FalseKeyword: {
                const falseLiteralExpr = new FalseLiteralExpression();
                falseLiteralExpr.setExprType(
                    this.typeResolver.generateNodeType(node),
                );
                return falseLiteralExpr;
            }
            case ts.SyntaxKind.TrueKeyword: {
                const trueLiteralExpr = new TrueLiteralExpression();
                trueLiteralExpr.setExprType(
                    this.typeResolver.generateNodeType(node),
                );
                return trueLiteralExpr;
            }
            case ts.SyntaxKind.Identifier: {
                const targetIdentifier = (<ts.Identifier>node).getText();
                const identifierExpr = new IdentifierExpression(
                    targetIdentifier,
                );
                let scope = this.parserCtx.getScopeByNode(node) || null;
                const varReferenceScope = scope!.getNearestFunctionScope();
                let variable: Variable | undefined = undefined;
                let maybeClosureVar = false;

                if (varReferenceScope) {
                    while (scope) {
                        variable = scope.findVariable(targetIdentifier, false);
                        if (variable) {
                            break;
                        }

                        if (varReferenceScope === scope) {
                            /* Variable not found in current function,
                                it may be a closure var, but we still need
                                to check if it's a global var */
                            maybeClosureVar = true;
                        }
                        scope = scope.parent;
                    }

                    if (maybeClosureVar) {
                        if (scope && scope.getNearestFunctionScope()) {
                            variable!.setVarIsClosure();
                            (scope as ClosureEnvironment).hasFreeVar = true;
                        }
                    }
                }
                /** in order to avoid there is narrowed type checking scope */
                let declNode = node;
                const symbol =
                    this.typeResolver.typechecker!.getSymbolAtLocation(node);
                if (symbol && symbol.valueDeclaration) {
                    declNode = symbol.valueDeclaration;
                }
                identifierExpr.setExprType(
                    this.typeResolver.generateNodeType(declNode),
                );
                return identifierExpr;
            }
            case ts.SyntaxKind.BinaryExpression: {
                const binaryExprNode = <ts.BinaryExpression>node;
                const leftExpr = this.visitNode(binaryExprNode.left);
                const rightExpr = this.visitNode(binaryExprNode.right);
                let expr: Expression = new BinaryExpression(
                    binaryExprNode.operatorToken.kind,
                    leftExpr,
                    rightExpr,
                );
                if (
                    ts.isPropertyAccessExpression(binaryExprNode.left) &&
                    binaryExprNode.operatorToken.kind ===
                        ts.SyntaxKind.EqualsToken
                ) {
                    const symbol =
                        this.parserCtx.typeChecker!.getSymbolAtLocation(
                            binaryExprNode.left.name,
                        );
                    if (symbol && symbol.declarations) {
                        const symbolDecls = symbol.declarations;
                        const hasSetter = symbolDecls.find((d) => {
                            return d.kind === ts.SyntaxKind.SetAccessor;
                        });
                        if (hasSetter) {
                            (
                                leftExpr as PropertyAccessExpression
                            ).accessSetter = true;
                            expr = new CallExpression(leftExpr, [rightExpr]);
                            const type = new TSFunction();
                            type.addParamType(
                                this.typeResolver.generateNodeType(
                                    binaryExprNode.left.name,
                                ),
                            );
                            leftExpr.setExprType(type);
                        }
                    }
                }
                expr.setExprType(this.typeResolver.generateNodeType(node));
                return expr;
            }
            case ts.SyntaxKind.PrefixUnaryExpression: {
                const prefixExprNode = <ts.PrefixUnaryExpression>node;
                const operand = this.visitNode(prefixExprNode.operand);
                const unaryExpr = new UnaryExpression(
                    ts.SyntaxKind.PrefixUnaryExpression,
                    prefixExprNode.operator,
                    operand,
                );
                unaryExpr.setExprType(this.typeResolver.generateNodeType(node));
                return unaryExpr;
            }
            case ts.SyntaxKind.PostfixUnaryExpression: {
                const postExprNode = <ts.PostfixUnaryExpression>node;
                const operand = this.visitNode(postExprNode.operand);
                const unaryExpr = new UnaryExpression(
                    ts.SyntaxKind.PostfixUnaryExpression,
                    postExprNode.operator,
                    operand,
                );
                unaryExpr.setExprType(this.typeResolver.generateNodeType(node));
                return unaryExpr;
            }
            case ts.SyntaxKind.ConditionalExpression: {
                const condExprNode = <ts.ConditionalExpression>node;
                const cond = this.visitNode(condExprNode.condition);
                const whenTrue = this.visitNode(condExprNode.whenTrue);
                const whenFalse = this.visitNode(condExprNode.whenFalse);
                const conditionalExpr = new ConditionalExpression(
                    cond,
                    whenTrue,
                    whenFalse,
                );
                conditionalExpr.setExprType(
                    this.typeResolver.generateNodeType(node),
                );
                return conditionalExpr;
            }
            case ts.SyntaxKind.CallExpression: {
                const callExprNode = <ts.CallExpression>node;
                const expr = this.visitNode(callExprNode.expression);
                const args = new Array<Expression>(
                    callExprNode.arguments.length,
                );
                for (let i = 0; i != args.length; ++i) {
                    args[i] = this.visitNode(callExprNode.arguments[i]);
                }
                if (
                    callExprNode.expression.kind === ts.SyntaxKind.SuperKeyword
                ) {
                    const callExpr = new SuperCallExpression(args);
                    callExpr.setExprType(
                        this.typeResolver.generateNodeType(node),
                    );
                    return callExpr;
                }
                const callExpr = new CallExpression(expr, args);
                callExpr.setExprType(this.typeResolver.generateNodeType(node));
                return callExpr;
            }
            case ts.SyntaxKind.PropertyAccessExpression: {
                const propAccessExprNode = <ts.PropertyAccessExpression>node;
                const expr = this.visitNode(propAccessExprNode.expression);
                const property = this.visitNode(propAccessExprNode.name);
                const propAccessExpr = new PropertyAccessExpression(
                    expr,
                    property,
                );
                propAccessExpr.setExprType(
                    this.typeResolver.generateNodeType(node),
                );
                return propAccessExpr;
            }
            case ts.SyntaxKind.ParenthesizedExpression: {
                const expr = this.visitNode(
                    (<ts.ParenthesizedExpression>node).expression,
                );
                const parentesizedExpr = new ParenthesizedExpression(expr);
                parentesizedExpr.setExprType(
                    this.typeResolver.generateNodeType(node),
                );
                return parentesizedExpr;
            }
            case ts.SyntaxKind.NewExpression: {
                const newExprNode = <ts.NewExpression>node;
                const expr = this.visitNode(newExprNode.expression);
                const newExpr = new NewExpression(expr);
                if (
                    expr.expressionKind === ts.SyntaxKind.Identifier &&
                    (<IdentifierExpression>expr).identifierName === 'Array'
                ) {
                    if (!newExprNode.typeArguments) {
                        throw new Error(
                            'new Array without declare element type',
                        );
                    }
                    let isLiteral = false;
                    if (newExprNode.arguments) {
                        /* Check if it's created from a literal */
                        const argLen = newExprNode.arguments.length;
                        if (argLen > 1) {
                            isLiteral = true;
                        } else if (argLen === 1) {
                            const elem = newExprNode.arguments[0];
                            const elemExpr = this.visitNode(elem);
                            if (elemExpr.exprType.kind !== TypeKind.NUMBER) {
                                isLiteral = true;
                            }
                        }

                        if (isLiteral) {
                            const elemExprs = newExprNode.arguments.map((a) => {
                                return this.visitNode(a);
                            });
                            newExpr.setArrayLen(argLen);
                            newExpr.setArgs(elemExprs);
                        } else if (argLen === 1) {
                            newExpr.setLenExpr(
                                this.visitNode(newExprNode.arguments[0]),
                            );
                        }
                        /* else no arguments */
                    }
                    newExpr.setExprType(
                        this.typeResolver.generateNodeType(node),
                    );
                    return newExpr;
                }
                if (newExprNode.arguments !== undefined) {
                    const args = new Array<Expression>();
                    for (const arg of newExprNode.arguments) {
                        args.push(this.visitNode(arg));
                    }
                    newExpr.setArgs(args);
                }
                newExpr.setExprType(this.typeResolver.generateNodeType(node));
                return newExpr;
            }
            case ts.SyntaxKind.ObjectLiteralExpression: {
                const objLiteralNode = <ts.ObjectLiteralExpression>node;
                const fields = new Array<IdentifierExpression>();
                const values = new Array<Expression>();
                for (const property of objLiteralNode.properties) {
                    const propertyAssign = <ts.PropertyAssignment>property;
                    fields.push(
                        new IdentifierExpression(propertyAssign.name.getText()),
                    );
                    values.push(this.visitNode(propertyAssign.initializer));
                }
                const objLiteralExpr = new ObjectLiteralExpression(
                    fields,
                    values,
                );
                objLiteralExpr.setExprType(
                    this.typeResolver.generateNodeType(node),
                );
                return objLiteralExpr;
            }
            case ts.SyntaxKind.ArrayLiteralExpression: {
                const arrLiteralNode = <ts.ArrayLiteralExpression>node;
                const elements = new Array<Expression>();
                for (const elem of arrLiteralNode.elements) {
                    elements.push(this.visitNode(elem));
                }
                const arrLiteralExpr = new ArrayLiteralExpression(elements);
                arrLiteralExpr.setExprType(
                    this.typeResolver.generateNodeType(node),
                );
                return arrLiteralExpr;
            }
            case ts.SyntaxKind.AsExpression: {
                const asExprNode = <ts.AsExpression>node;
                const expr = this.visitNode(asExprNode.expression);
                const typeNode = asExprNode.type;
                const asExpr = new AsExpression(expr);
                asExpr.setExprType(
                    this.typeResolver.generateNodeType(typeNode),
                );
                return asExpr;
            }
            case ts.SyntaxKind.ElementAccessExpression: {
                const elementAccessExprNode = <ts.ElementAccessExpression>node;
                const expr = this.visitNode(elementAccessExprNode.expression);
                const argExpr = this.visitNode(
                    elementAccessExprNode.argumentExpression,
                );
                const elementAccessExpr = new ElementAccessExpression(
                    expr,
                    argExpr,
                );
                elementAccessExpr.setExprType(
                    this.typeResolver.generateNodeType(node),
                );
                return elementAccessExpr;
            }
            case ts.SyntaxKind.FunctionExpression:
            case ts.SyntaxKind.ArrowFunction: {
                const funcScope = getCurScope(node, this.nodeScopeMap)!;
                const funcExpr = new FunctionExpression(
                    funcScope as FunctionScope,
                );
                funcExpr.setExprType(this.typeResolver.generateNodeType(node));
                return funcExpr;
            }
            case ts.SyntaxKind.ThisKeyword: {
                const expr = new IdentifierExpression('this');
                expr.setExprType(this.typeResolver.generateNodeType(node));
                return expr;
            }
            default:
                Logger.warn(
                    `Encounter un-processed expression, kind: ${node.kind}`,
                );
                return new Expression(ts.SyntaxKind.Unknown);
        }
    }
}
