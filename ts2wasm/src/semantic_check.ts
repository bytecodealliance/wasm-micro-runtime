/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import {
    Scope,
    FunctionScope,
    GlobalScope,
    BlockScope,
    ClassScope,
    NamespaceScope,
} from './scope.js';
import {
    TSClass,
    TSInterface,
    Type,
    TypeKind,
    TSArray,
    TSFunction,
    Primitive,
} from './type.js';
import { Logger } from './log.js';
import { ParserContext } from './frontend.js';
import {
    BinaryExpression,
    CallExpression,
    Expression,
    PropertyAccessExpression,
} from './expression.js';
import { Parameter, Variable } from './variable.js';
import {
    BaseLoopStatement,
    CaseClause,
    ExpressionStatement,
    ForStatement,
    IfStatement,
    ReturnStatement,
    Statement,
    SwitchStatement,
} from './statement.js';

const enum ErrorKind {
    NominalClass = 'nominal class',
    InnerFuncDefaultParam = 'inner closure with default parameters',
    nonAnyAndAny = 'operate between non-any type and any type',
    OperateDiffTypes = 'operate between different types',
    InvokeAnyObj = 'invoke any object',
}

const enum ErrorFlag {
    BinaryOperationOnNominalClass,
    BinaryOperationOnNonAnyAndAnyType,
    BinaryOperationOnDifferentTypes,
    ReturnTypesAreNominalClass,
    ReturnTypesAreNonAnyAndAnyTypes,
    InnerFuncHasDefaultParams,
    InvokeAnyObject,
    ArgsAndParamsTypesAreNominalClass,
    ArgsAndParamsTypesAreNonAnyAndAnyTypes,
}

interface SematicError {
    errorKind: ErrorKind;
    errorFlag: ErrorFlag;
    message: string;
    scopeName: string;
}

export default class SemanticChecker {
    private errors: SematicError[] = [];
    private curScope: Scope | null = null;
    private globalScopes;

    constructor(private parserCtx: ParserContext) {
        this.globalScopes = parserCtx.globalScopes;
    }

    sematicCheck() {
        for (let i = 0; i < this.globalScopes.length; ++i) {
            const scope = this.globalScopes[i];
            scope.traverseScopTree((scope) => {
                this.curScope = scope;
                scope.varArray.map((v) => {
                    this.exprAccept(v.initExpression);
                    this.varInitAccept(v);
                });
                if (scope instanceof FunctionScope) {
                    let hasDefaultParam = false;
                    scope.paramArray.map((p) => {
                        const init = p.initExpression;
                        if (init) {
                            hasDefaultParam = true;
                        }
                        this.exprAccept(init);
                        this.varInitAccept(p);
                    });
                    if (hasDefaultParam) {
                        this.defaultParamAccept();
                    }
                }

                scope.statements.map((stmt) => {
                    this.stmtAccept(stmt);
                });
                this.curScope = scope.parent;
            });
        }
        if (this.errors.length > 0) {
            Logger.error(this.logErrors());
            throw new Error('Semantic check error.');
        }
    }

    private stmtAccept(stmt: Statement) {
        let expr: Expression | null = null;
        if (stmt instanceof IfStatement) {
            expr = stmt.ifCondition;
        } else if (stmt instanceof ReturnStatement) {
            expr = stmt.returnExpression;
        } else if (stmt instanceof BaseLoopStatement) {
            expr = stmt.loopCondtion;
        } else if (stmt instanceof ForStatement) {
            expr = stmt.forLoopCondtion;
        } else if (stmt instanceof ExpressionStatement) {
            expr = stmt.expression;
        } else if (stmt instanceof CaseClause) {
            expr = stmt.caseExpr;
        } else if (stmt instanceof SwitchStatement) {
            expr = stmt.switchCondition;
        }
        this.exprAccept(expr);
        if (stmt instanceof ForStatement) {
            this.exprAccept(stmt.forLoopIncrementor);
        }
        if (stmt instanceof ReturnStatement) {
            this.returnTypeCheck(expr!);
        }
    }

    private exprAccept(expr: Expression | null) {
        if (expr instanceof CallExpression) {
            this.checkCallExpr(expr);
        } else if (expr instanceof BinaryExpression) {
            this.binaryOperateCheck(
                expr.leftOperand.exprType,
                expr.rightOperand.exprType,
                expr.operatorKind,
            );
            if (expr.rightOperand instanceof PropertyAccessExpression) {
                this.invokeAnyObjCheck(
                    expr.leftOperand.exprType,
                    expr.rightOperand.propertyAccessExpr.exprType,
                );
            }
        }
    }

    private varInitAccept(expr: Variable | Parameter) {
        if (expr.initExpression) {
            this.binaryOperateCheck(
                expr.varType,
                expr.initExpression.exprType,
                ts.SyntaxKind.EqualsToken,
            );
            if (expr.initExpression instanceof PropertyAccessExpression) {
                this.invokeAnyObjCheck(
                    expr.varType,
                    expr.initExpression.propertyAccessExpr.exprType,
                );
            }
        }
    }

    // check arguments and parameters types
    private checkCallExpr(expr: CallExpression) {
        const funcType = expr.callExpr.exprType as TSFunction;
        let paramTypes = funcType.getParamTypes();
        if (funcType.hasRest()) {
            const restType = (<TSArray>paramTypes[paramTypes.length - 1])
                .elementType;
            if (paramTypes.length <= expr.callArgs.length) {
                const restTypes = new Array<Type>(
                    expr.callArgs.length - paramTypes.length + 1,
                ).fill(restType);
                paramTypes = paramTypes
                    .slice(0, paramTypes.length - 1)
                    .concat(restTypes);
            }
        }

        for (let i = 0; i < expr.callArgs.length; i++) {
            const paramType = paramTypes[i];
            const argExpr = expr.callArgs[i];
            this.nominalClassCheck(
                paramType,
                argExpr.exprType,
                ErrorFlag.ArgsAndParamsTypesAreNominalClass,
                `argument type and parameter type are nominal class types`,
            );
            this.nonAnyAndAnyCheck(
                paramType,
                argExpr.exprType,
                ErrorFlag.ArgsAndParamsTypesAreNonAnyAndAnyTypes,
                `explicitly pass any type as argument without type casting`,
            );
            if (argExpr instanceof PropertyAccessExpression) {
                this.invokeAnyObjCheck(
                    paramType,
                    argExpr.propertyAccessExpr.exprType,
                );
            }
        }
    }

    // check return statement type and function return type
    private returnTypeCheck(expr: Expression) {
        const funcScope =
            this.curScope!.getNearestFunctionScope() as FunctionScope;
        const returnType = funcScope.funcType.returnType;
        this.nominalClassCheck(
            returnType,
            expr.exprType,
            ErrorFlag.ReturnTypesAreNominalClass,
            `return statement type and function return type are nominal classes`,
        );
        this.nonAnyAndAnyCheck(
            returnType,
            expr.exprType,
            ErrorFlag.ReturnTypesAreNonAnyAndAnyTypes,
            `return statement type and function return type are non-any and any types`,
        );
        if (expr instanceof PropertyAccessExpression) {
            this.invokeAnyObjCheck(
                returnType,
                expr.propertyAccessExpr.exprType,
            );
        }
    }

    private binaryOperateCheck(
        left: Type,
        right: Type,
        operateKind: ts.SyntaxKind,
    ) {
        this.nominalClassCheck(
            left,
            right,
            ErrorFlag.BinaryOperationOnNominalClass,
            `binary operation between different nominal classes`,
        );
        this.nonAnyAndAnyCheck(
            left,
            right,
            ErrorFlag.BinaryOperationOnNonAnyAndAnyType,
            `binary operation between non-any type and any type without type cast`,
        );
        this.diffTypesOprtCheck(
            left,
            right,
            ErrorFlag.BinaryOperationOnDifferentTypes,
            operateKind,
        );
    }

    private defaultParamAccept() {
        const parent = this.curScope!.parent;
        const parentLevelFuncScope = parent!.getNearestFunctionScope();
        if (parentLevelFuncScope) {
            this.errors.push({
                errorKind: ErrorKind.InnerFuncDefaultParam,
                errorFlag: ErrorFlag.InnerFuncHasDefaultParams,
                message: `inner function has default parameters`,
                scopeName: this.getScopeName(this.curScope!),
            });
        }
    }

    /** addtional sematic checking rules */
    private nominalClassCheck(
        left: Type,
        right: Type,
        flag: ErrorFlag,
        msg: string,
    ) {
        if (left instanceof TSInterface || right instanceof TSInterface) {
            return;
        }
        if (!(left instanceof TSClass) || !(right instanceof TSClass)) {
            return;
        }
        if (left.className !== right.className) {
            // is downcast
            // TODO: classname maybe can not get right result when envolving with multiple modules
            let base = right.getBase();
            while (base) {
                if (base.className === left.className) {
                    return;
                }
                base = base.getBase();
            }
            base = left.getBase();
            while (base) {
                if (base.className === right.className) {
                    return;
                }
                base = base.getBase();
            }
            this.errors.push({
                errorKind: ErrorKind.NominalClass,
                errorFlag: flag,
                message: msg,
                scopeName: this.getScopeName(this.curScope!),
            });
        }
    }

    private nonAnyAndAnyCheck(
        left: Type,
        right: Type,
        flag: number,
        msg: string,
    ) {
        const cond =
            left.kind !== TypeKind.ANY &&
            !(left instanceof Primitive) &&
            right.kind === TypeKind.ANY;
        if (cond) {
            this.errors.push({
                errorKind: ErrorKind.nonAnyAndAny,
                errorFlag: flag,
                message: msg,
                scopeName: this.getScopeName(this.curScope!),
            });
        }
    }

    private invokeAnyObjCheck(left: Type, exprType: Type) {
        if (left.kind !== TypeKind.ANY && exprType.kind === TypeKind.ANY) {
            this.errors.push({
                errorKind: ErrorKind.InvokeAnyObj,
                errorFlag: ErrorFlag.InvokeAnyObject,
                message: `invoke any object without cast to a specific type`,
                scopeName: this.getScopeName(this.curScope!),
            });
        }
    }

    // current checking `number + string` and `string + number`
    private diffTypesOprtCheck(
        left: Type,
        right: Type,
        flag: ErrorFlag,
        operator: ts.SyntaxKind,
    ) {
        if (
            operator === ts.SyntaxKind.PlusToken ||
            operator === ts.SyntaxKind.PlusEqualsToken
        ) {
            if (
                (left.kind == TypeKind.NUMBER &&
                    right.kind == TypeKind.STRING) ||
                (left.kind == TypeKind.STRING && right.kind == TypeKind.NUMBER)
            )
                this.errors.push({
                    errorKind: ErrorKind.OperateDiffTypes,
                    errorFlag: flag,
                    message: `binary operation between ${left.kind} and ${right.kind}`,
                    scopeName: this.getScopeName(this.curScope!),
                });
        }
    }

    private logErrors() {
        let res = 'sematic checking not passing: \n';
        for (const error of this.errors) {
            res += `[${error.errorKind}]: in [${error.scopeName}], error flag: '${error.errorFlag}', message: '${error.message}' \n`;
        }
        return res;
    }

    private getScopeName(scope: Scope): string {
        if (scope instanceof FunctionScope) {
            return `${scope.mangledName}`;
        }
        if (scope instanceof GlobalScope) {
            return `${scope.mangledName}`;
        }
        if (scope instanceof BlockScope) {
            const nearstScope = scope.getNearestFunctionScope();
            // global scope
            if (nearstScope === null) {
                return `${scope.mangledName}`;
            } else {
                return this.getScopeName(nearstScope);
            }
        }
        if (scope instanceof ClassScope) {
            return `${scope.mangledName}`;
        }
        if (scope instanceof NamespaceScope) {
            return `${scope.mangledName}`;
        }
        return 'unknown scope';
    }
}
