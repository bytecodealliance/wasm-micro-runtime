/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import { assert } from 'console';
import { ParserContext } from './frontend.js';
import {
    BinaryExpression,
    Expression,
    IdentifierExpression,
    PropertyAccessExpression,
} from './expression.js';
import { FunctionScope, Scope, ScopeKind } from './scope.js';
import {
    parentIsFunctionLike,
    Stack,
    getImportModulePath,
    getGlobalScopeByModuleName,
    importGlobalInfo,
    importFunctionInfo,
} from './utils.js';
import { Variable } from './variable.js';
import { BuiltinNames } from '../lib/builtin/builtin_name.js';
import { TSClass, Type } from './type.js';
import { Logger } from './log.js';

type StatementKind = ts.SyntaxKind;

export class Statement {
    private _scope: Scope | null = null;

    constructor(private kind: StatementKind) {}

    get statementKind(): StatementKind {
        return this.kind;
    }

    setScope(scope: Scope) {
        this._scope = scope;
    }

    getScope(): Scope | null {
        return this._scope;
    }
}

export class IfStatement extends Statement {
    constructor(
        private condition: Expression,
        private ifTrue: Statement,
        private ifFalse: Statement | null,
    ) {
        super(ts.SyntaxKind.IfStatement);
    }

    get ifCondition(): Expression {
        return this.condition;
    }

    get ifIfTrue(): Statement {
        return this.ifTrue;
    }

    get ifIfFalse(): Statement | null {
        return this.ifFalse;
    }
}

export class BlockStatement extends Statement {
    constructor() {
        super(ts.SyntaxKind.Block);
    }
}

export class ReturnStatement extends Statement {
    constructor(private expr: Expression | null) {
        super(ts.SyntaxKind.ReturnStatement);
    }

    get returnExpression(): Expression | null {
        return this.expr;
    }
}

// create 'while' or 'do...while' loop
export class BaseLoopStatement extends Statement {
    constructor(
        kind: StatementKind,
        private _loopLabel: string,
        private _blockLabel: string,
        private cond: Expression,
        private body: Statement,
    ) {
        super(kind);
    }

    get loopLabel(): string {
        return this._loopLabel;
    }

    get loopBlockLabel(): string {
        return this._blockLabel;
    }

    get loopCondtion(): Expression {
        return this.cond;
    }

    get loopBody(): Statement {
        return this.body;
    }
}

export class ForStatement extends Statement {
    constructor(
        private label: string,
        private blockLabel: string,
        private cond: Expression | null,
        private body: Statement,
        /** VariableStatement or ExpressionStatement */
        private initializer: Statement | null,
        private incrementor: Expression | null,
    ) {
        super(ts.SyntaxKind.ForStatement);
    }

    get forLoopLabel(): string {
        return this.label;
    }

    get forLoopBlockLabel(): string {
        return this.blockLabel;
    }

    get forLoopCondtion(): Expression | null {
        return this.cond;
    }

    get forLoopBody(): Statement {
        return this.body;
    }

    get forLoopInitializer(): Statement | null {
        return this.initializer;
    }

    get forLoopIncrementor(): Expression | null {
        return this.incrementor;
    }
}

export class ExpressionStatement extends Statement {
    constructor(private expr: Expression) {
        super(ts.SyntaxKind.ExpressionStatement);
    }

    get expression(): Expression {
        return this.expr;
    }
}

export class EmptyStatement extends Statement {
    constructor() {
        super(ts.SyntaxKind.EmptyStatement);
    }
}

export class CaseClause extends Statement {
    constructor(private expr: Expression, private statements: Statement[]) {
        super(ts.SyntaxKind.CaseClause);
    }

    get caseExpr(): Expression {
        return this.expr;
    }

    get caseStatements(): Statement[] {
        return this.statements;
    }
}

export class DefaultClause extends Statement {
    constructor(private statements: Statement[]) {
        super(ts.SyntaxKind.DefaultClause);
    }

    get caseStatements(): Statement[] {
        return this.statements;
    }
}

export class CaseBlock extends Statement {
    constructor(
        private _switchLabel: string,
        private _breakLabel: string,
        private causes: Statement[],
    ) {
        super(ts.SyntaxKind.CaseBlock);
    }

    get switchLabel(): string {
        return this._switchLabel;
    }

    get breakLabel(): string {
        return this._breakLabel;
    }

    get caseCauses(): Statement[] {
        return this.causes;
    }
}
export class SwitchStatement extends Statement {
    constructor(private cond: Expression, private caseBlock: Statement) {
        super(ts.SyntaxKind.SwitchStatement);
    }

    get switchCondition(): Expression {
        return this.cond;
    }

    get switchCaseBlock(): Statement {
        return this.caseBlock;
    }
}

export class BreakStatement extends Statement {
    constructor(private label: string) {
        super(ts.SyntaxKind.BreakStatement);
    }

    get breakLabel(): string {
        return this.label;
    }
}

export class VariableStatement extends Statement {
    private variableArray: Variable[] = [];

    constructor() {
        super(ts.SyntaxKind.VariableStatement);
    }

    addVariable(variable: Variable) {
        this.variableArray.push(variable);
    }

    get varArray(): Variable[] {
        return this.variableArray;
    }
}

export class ImportDeclaration extends Statement {
    importModuleStartFuncName = '';

    constructor() {
        super(ts.SyntaxKind.ImportDeclaration);
    }
}

export default class StatementProcessor {
    private loopLabelStack = new Stack<string>();
    private breakLabelsStack = new Stack<string>();
    private switchLabelStack = new Stack<number>();
    private currentScope: Scope | null = null;

    constructor(private parserCtx: ParserContext) {}

    visit() {
        this.parserCtx.nodeScopeMap.forEach((scope, node) => {
            this.currentScope = scope;
            /** arrow function body is a ts.expression */
            if (ts.isArrowFunction(node) && !ts.isBlock(node.body)) {
                const expr = this.parserCtx.expressionProcessor.visitNode(
                    node.body,
                );
                scope.addStatement(new ReturnStatement(expr));
            }
            /* During the traverse, it will enter the inner
            block scope, so we skip BlockScope here */
            if (
                scope.kind !== ScopeKind.BlockScope &&
                scope.kind !== ScopeKind.ClassScope
            ) {
                ts.forEachChild(node, (node) => {
                    const stmt = this.visitNode(node);
                    if (stmt) {
                        scope.addStatement(stmt);
                    }
                });
            }
        });
    }

    visitNode(node: ts.Node): Statement | null {
        switch (node.kind) {
            case ts.SyntaxKind.ImportDeclaration: {
                const importDeclaration = <ts.ImportDeclaration>node;
                // Get the import module name according to the relative position of current scope
                const importModuleName = getImportModulePath(
                    importDeclaration,
                    this.currentScope!.getRootGloablScope()!,
                );
                const importModuleScope = getGlobalScopeByModuleName(
                    importModuleName,
                    this.parserCtx.globalScopes,
                );
                const importStmt = new ImportDeclaration();
                if (!importModuleScope.isCircularImport) {
                    importStmt.importModuleStartFuncName =
                        importModuleScope.startFuncName;
                    importModuleScope.isCircularImport = true;
                    return importStmt;
                }
                /** Currently, we put all ts files into a whole wasm file.
                 *  So we don't need to collect import information here.
                 *  If we generate several wasm files, we need to collect here.
                 */
                return importStmt;
            }
            case ts.SyntaxKind.VariableStatement: {
                const varStatementNode = <ts.VariableStatement>node;
                const varStatement = new VariableStatement();
                const varDeclarationList = varStatementNode.declarationList;
                this.currentScope = this.parserCtx.getScopeByNode(node)!;
                this.addVariableInVarStmt(
                    varDeclarationList,
                    varStatement,
                    this.currentScope,
                );
                return varStatement;
            }
            case ts.SyntaxKind.IfStatement: {
                const ifStatementNode = <ts.IfStatement>node;
                const condtion: Expression =
                    this.parserCtx.expressionProcessor.visitNode(
                        ifStatementNode.expression,
                    );
                const ifTrue: Statement = this.visitNode(
                    ifStatementNode.thenStatement,
                )!;
                const ifFalse: Statement | null = ifStatementNode.elseStatement
                    ? this.visitNode(ifStatementNode.elseStatement)
                    : null;
                const ifStmt = new IfStatement(condtion, ifTrue, ifFalse);
                return ifStmt;
            }
            case ts.SyntaxKind.Block: {
                /* every ts.Block(except function.body) has a corresponding block scope and BlockStatement */
                const blockNode = <ts.Block>node;
                const scope = this.parserCtx.getScopeByNode(blockNode)!;

                for (const stmt of blockNode.statements) {
                    const compiledStmt = this.visitNode(stmt)!;
                    if (!compiledStmt) {
                        continue;
                    }
                    scope.addStatement(compiledStmt);
                }

                /* Block of function scope, just add statements to parent scope,
                    don't create a new BlockStatement */
                if (parentIsFunctionLike(node)) {
                    return null;
                }

                const block = new BlockStatement();
                block.setScope(scope);
                return block;
            }
            case ts.SyntaxKind.ReturnStatement: {
                const returnStatementNode = <ts.ReturnStatement>node;
                const retStmt = new ReturnStatement(
                    returnStatementNode.expression
                        ? this.parserCtx.expressionProcessor.visitNode(
                              returnStatementNode.expression,
                          )
                        : null,
                );
                return retStmt;
            }
            case ts.SyntaxKind.WhileStatement: {
                const whileStatementNode = <ts.WhileStatement>node;
                this.currentScope = this.parserCtx.getScopeByNode(node)!;
                const scope = this.currentScope;
                const loopLabel = 'while_loop_' + this.loopLabelStack.size();
                const breakLabels = this.breakLabelsStack;
                breakLabels.push(loopLabel + 'block');
                const blockLabel = breakLabels.peek();
                this.loopLabelStack.push(loopLabel);

                const expr = this.parserCtx.expressionProcessor.visitNode(
                    whileStatementNode.expression,
                );
                const statement = this.visitNode(whileStatementNode.statement)!;
                this.breakLabelsStack.pop();
                const loopStatment = new BaseLoopStatement(
                    ts.SyntaxKind.WhileStatement,
                    loopLabel,
                    blockLabel,
                    expr,
                    statement,
                );
                loopStatment.setScope(scope);
                return loopStatment;
            }
            case ts.SyntaxKind.DoStatement: {
                const doWhileStatementNode = <ts.DoStatement>node;
                this.currentScope = this.parserCtx.getScopeByNode(node)!;
                const scope = this.currentScope;
                const loopLabel = 'do_loop_' + this.loopLabelStack.size();
                const breakLabels = this.breakLabelsStack;
                breakLabels.push(loopLabel + 'block');
                const blockLabel = breakLabels.peek();
                this.loopLabelStack.push(loopLabel);

                const expr = this.parserCtx.expressionProcessor.visitNode(
                    doWhileStatementNode.expression,
                );
                const statement = this.visitNode(
                    doWhileStatementNode.statement,
                )!;
                this.breakLabelsStack.pop();
                const loopStatment = new BaseLoopStatement(
                    ts.SyntaxKind.DoStatement,
                    loopLabel,
                    blockLabel,
                    expr,
                    statement,
                );
                loopStatment.setScope(scope);
                return loopStatment;
            }
            case ts.SyntaxKind.ForStatement: {
                const forStatementNode = <ts.ForStatement>node;
                this.currentScope = this.parserCtx.getScopeByNode(node)!;
                const scope = this.currentScope;
                const loopLabel = 'for_loop_' + this.loopLabelStack.size();
                const breakLabels = this.breakLabelsStack;
                breakLabels.push(loopLabel + 'block');
                const blockLabel = breakLabels.peek();
                this.loopLabelStack.push(loopLabel);

                let initializer = null;
                if (forStatementNode.initializer) {
                    const forInit = forStatementNode.initializer;
                    let initStmt: Statement;
                    if (ts.isVariableDeclarationList(forInit)) {
                        initStmt = new VariableStatement();
                        this.addVariableInVarStmt(
                            forInit,
                            initStmt as VariableStatement,
                            scope,
                        );
                    } else {
                        initStmt = new ExpressionStatement(
                            this.parserCtx.expressionProcessor.visitNode(
                                forInit,
                            ),
                        );
                    }
                    initializer = initStmt;
                }

                const cond = forStatementNode.condition
                    ? this.parserCtx.expressionProcessor.visitNode(
                          forStatementNode.condition,
                      )
                    : null;
                const incrementor = forStatementNode.incrementor
                    ? this.parserCtx.expressionProcessor.visitNode(
                          forStatementNode.incrementor,
                      )
                    : null;
                const statement = this.visitNode(forStatementNode.statement)!;
                this.breakLabelsStack.pop();
                const forStatement = new ForStatement(
                    loopLabel,
                    blockLabel,
                    cond,
                    statement,
                    initializer,
                    incrementor,
                );
                forStatement.setScope(scope);
                return forStatement;
            }
            case ts.SyntaxKind.ExpressionStatement: {
                const exprStatement = <ts.ExpressionStatement>node;
                const exprStmt = new ExpressionStatement(
                    this.parserCtx.expressionProcessor.visitNode(
                        exprStatement.expression,
                    ),
                );
                return exprStmt;
            }
            case ts.SyntaxKind.EmptyStatement: {
                const emptyStmt = new EmptyStatement();
                return emptyStmt;
            }
            case ts.SyntaxKind.SwitchStatement: {
                const switchStatementNode = <ts.SwitchStatement>node;
                const switchLabels = this.switchLabelStack;
                switchLabels.push(switchLabels.size());
                const breakLabels = this.breakLabelsStack;
                breakLabels.push('break-switch-' + switchLabels.size());
                const expr = this.parserCtx.expressionProcessor.visitNode(
                    switchStatementNode.expression,
                );
                const caseBlock = this.visitNode(
                    switchStatementNode.caseBlock,
                )!;
                switchLabels.pop();
                breakLabels.pop();
                const swicthStmt = new SwitchStatement(expr, caseBlock);
                return swicthStmt;
            }
            case ts.SyntaxKind.CaseBlock: {
                const caseBlockNode = <ts.CaseBlock>node;
                this.currentScope = this.parserCtx.getScopeByNode(node)!;
                const scope = this.currentScope;
                const breakLabelsStack = this.breakLabelsStack;
                const switchLabels = this.switchLabelStack;
                const switchLabel = '_' + switchLabels.peek().toString();

                const clauses = new Array<Statement>();
                for (let i = 0; i !== caseBlockNode.clauses.length; ++i) {
                    clauses.push(this.visitNode(caseBlockNode.clauses[i])!);
                }
                const caseBlock = new CaseBlock(
                    switchLabel,
                    breakLabelsStack.peek(),
                    clauses,
                );
                caseBlock.setScope(scope);
                return caseBlock;
            }
            case ts.SyntaxKind.CaseClause: {
                const caseClauseNode = <ts.CaseClause>node;
                this.currentScope = this.parserCtx.getScopeByNode(node)!;
                const scope = this.currentScope;
                const expr = this.parserCtx.expressionProcessor.visitNode(
                    caseClauseNode.expression,
                );
                const statements = new Array<Statement>();
                const caseStatements = caseClauseNode.statements;
                for (let i = 0; i != caseStatements.length; ++i) {
                    statements.push(this.visitNode(caseStatements[i])!);
                }
                const caseCause = new CaseClause(expr, statements);
                caseCause.setScope(scope);
                return caseCause;
            }
            case ts.SyntaxKind.DefaultClause: {
                const defaultClauseNode = <ts.DefaultClause>node;
                this.currentScope = this.parserCtx.getScopeByNode(node)!;
                const scope = this.currentScope;
                const statements = new Array<Statement>();
                const caseStatements = defaultClauseNode.statements;
                for (let i = 0; i != caseStatements.length; ++i) {
                    statements.push(this.visitNode(caseStatements[i])!);
                }
                const defaultClause = new DefaultClause(statements);
                defaultClause.setScope(scope);
                return defaultClause;
            }
            case ts.SyntaxKind.BreakStatement: {
                const breakStatementNode = <ts.BreakStatement>node;
                assert(!breakStatementNode.label, 'not support goto');
                const breakStmt = new BreakStatement(
                    this.breakLabelsStack.peek(),
                );
                return breakStmt;
            }
            default:
                Logger.info(
                    `Encounter unprocessed statements, kind: [${node.kind}]`,
                );
                break;
        }

        return null;
    }

    addVariableInVarStmt(
        varDeclarationList: ts.VariableDeclarationList,
        varStatement: VariableStatement,
        currentScope: Scope,
    ) {
        for (const varDeclaration of varDeclarationList.declarations) {
            const varDecNode = <ts.VariableDeclaration>varDeclaration;
            const varName = (<ts.Identifier>varDecNode.name).getText()!;
            const variable = this.currentScope!.findVariable(varName);
            if (!variable) {
                throw new Error(
                    'can not find ' + varName + ' in current scope',
                );
            }
            varStatement.addVariable(variable);
            if (variable.isFuncScopedVar() && varDecNode.initializer) {
                const identifierExpr = new IdentifierExpression(varName);
                identifierExpr.setExprType(variable.varType);
                const initExpr = this.parserCtx.expressionProcessor.visitNode(
                    varDecNode.initializer,
                );
                const assignExpr = new BinaryExpression(
                    ts.SyntaxKind.EqualsToken,
                    identifierExpr,
                    initExpr,
                );
                assignExpr.setExprType(variable.varType);
                const expressionStmt = new ExpressionStatement(assignExpr);
                currentScope.addStatement(expressionStmt);
            }
        }
    }

    createFieldAssignStmt(
        initializer: ts.Node,
        classType: TSClass,
        fieldType: Type,
        fieldName: string,
    ): Statement {
        const thisExpr = new IdentifierExpression('this');
        thisExpr.setExprType(classType);
        const fieldExpr = new IdentifierExpression(fieldName);
        fieldExpr.setExprType(fieldType);
        const propAccessExpr = new PropertyAccessExpression(
            thisExpr,
            fieldExpr,
        );
        propAccessExpr.setExprType(fieldType);
        const initExpr =
            this.parserCtx.expressionProcessor.visitNode(initializer);
        const assignExpr = new BinaryExpression(
            ts.SyntaxKind.EqualsToken,
            propAccessExpr,
            initExpr,
        );
        assignExpr.setExprType(fieldType);
        return new ExpressionStatement(assignExpr);
    }
}
