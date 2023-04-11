/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import binaryen from 'binaryen';
import * as binaryenCAPI from './glue/binaryen.js';
import {
    Statement,
    IfStatement,
    ReturnStatement,
    BaseLoopStatement,
    ForStatement,
    SwitchStatement,
    CaseBlock,
    CaseClause,
    BreakStatement,
    DefaultClause,
    BlockStatement,
    ExpressionStatement,
    VariableStatement,
    ImportDeclaration,
} from '../../statement.js';
import {
    FunctionScope,
    Scope,
    ClosureEnvironment,
    ScopeKind,
} from '../../scope.js';
import { typeInfo } from './glue/utils.js';
import { flattenLoopStatement, FlattenLoop } from './utils.js';
import { WASMGen } from './index.js';
import { TSClass, TSFunction, TypeKind } from '../../type.js';
import { assert } from 'console';
import { BuiltinNames } from '../../../lib/builtin/builtin_name.js';

export class WASMStatementGen {
    private currentFuncCtx;

    constructor(private WASMCompiler: WASMGen) {
        this.currentFuncCtx = WASMCompiler.curFunctionCtx!;
    }

    WASMStmtGen(stmt: Statement): binaryen.ExpressionRef {
        this.currentFuncCtx = this.WASMCompiler.curFunctionCtx!;

        switch (stmt.statementKind) {
            case ts.SyntaxKind.IfStatement: {
                const ifStmt = this.WASMIfStmt(<IfStatement>stmt);
                return ifStmt;
            }
            case ts.SyntaxKind.Block: {
                const blockStmt = this.WASMBlock(<BlockStatement>stmt);
                return blockStmt;
            }
            case ts.SyntaxKind.ReturnStatement: {
                const returnStmt = this.WASMReturnStmt(<ReturnStatement>stmt);
                return returnStmt;
            }
            case ts.SyntaxKind.EmptyStatement: {
                const emptyStmt = this.WASMEmptyStmt();
                return emptyStmt;
            }
            case ts.SyntaxKind.WhileStatement:
            case ts.SyntaxKind.DoStatement: {
                const loopStmt = this.WASMBaseLoopStmt(<BaseLoopStatement>stmt);
                return loopStmt;
            }
            case ts.SyntaxKind.ForStatement: {
                const forStmt = this.WASMForStmt(<ForStatement>stmt);
                return forStmt;
            }
            case ts.SyntaxKind.SwitchStatement: {
                const switchStmt = this.WASMSwitchStmt(<SwitchStatement>stmt);
                return switchStmt;
            }
            case ts.SyntaxKind.BreakStatement: {
                const breakStmt = this.WASMBreakStmt(<BreakStatement>stmt);
                return breakStmt;
            }
            case ts.SyntaxKind.ExpressionStatement: {
                const exprStmt = this.WASMExpressionStmt(
                    <ExpressionStatement>stmt,
                );
                return exprStmt;
            }
            case ts.SyntaxKind.VariableStatement: {
                const varStatement = this.WASMVarStmt(<VariableStatement>stmt);
                return varStatement;
            }
            case ts.SyntaxKind.ImportDeclaration: {
                const callStartStmt = this.WASMImportStmt(
                    <ImportDeclaration>stmt,
                );
                return callStartStmt;
            }
            default:
                throw new Error('unexpected expr kind ' + stmt.statementKind);
        }
    }

    WASMIfStmt(stmt: IfStatement): binaryen.ExpressionRef {
        let wasmCond: binaryen.ExpressionRef =
            this.WASMCompiler.wasmExpr.WASMExprGen(
                stmt.ifCondition,
            ).binaryenRef;
        wasmCond = this.WASMCompiler.wasmExpr.generateCondition(wasmCond);
        const wasmIfTrue: binaryen.ExpressionRef = this.WASMStmtGen(
            stmt.ifIfTrue,
        );
        if (stmt.ifIfFalse === null) {
            return this.WASMCompiler.module.if(wasmCond, wasmIfTrue);
        } else {
            const wasmIfFalse: binaryen.ExpressionRef = this.WASMStmtGen(
                stmt.ifIfFalse,
            );
            return this.WASMCompiler.module.if(
                wasmCond,
                wasmIfTrue,
                wasmIfFalse,
            );
        }
    }

    WASMBlock(stmt: BlockStatement): binaryen.ExpressionRef {
        const scope = stmt.getScope();
        if (scope === null) {
            throw new Error('BlockStatement corresponding scope is null');
        }

        this.currentFuncCtx.enterScope(scope);

        for (const stmt of scope.statements) {
            const stmtRef = this.WASMStmtGen(stmt);
            if (stmt.statementKind === ts.SyntaxKind.VariableStatement) {
                continue;
            }
            this.currentFuncCtx.insert(stmtRef);
        }

        return this.WASMCompiler.module.block(
            null,
            this.currentFuncCtx.exitScope(),
        );
    }

    WASMReturnStmt(stmt: ReturnStatement): binaryen.ExpressionRef {
        const module = this.WASMCompiler.module;
        const brReturn = module.br('statements');
        if (stmt.returnExpression === null) {
            return brReturn;
        }
        const curNearestFuncScope = this.currentFuncCtx.getFuncScope();
        assert(curNearestFuncScope instanceof FunctionScope);
        const nearestFuncScope = <FunctionScope>curNearestFuncScope;
        let returnExprRef: binaryen.ExpressionRef;
        const type = nearestFuncScope.funcType;
        if (
            type.returnType.kind === TypeKind.ANY &&
            stmt.returnExpression.exprType.kind !== TypeKind.ANY
        ) {
            returnExprRef =
                this.WASMCompiler.wasmDynExprCompiler.WASMDynExprGen(
                    stmt.returnExpression,
                ).binaryenRef;
        } else {
            returnExprRef = this.WASMCompiler.wasmExpr.WASMExprGen(
                stmt.returnExpression,
            ).binaryenRef;
            if (
                stmt.returnExpression.exprType instanceof TSClass &&
                nearestFuncScope.funcType.returnType instanceof TSClass
            ) {
                returnExprRef =
                    this.WASMCompiler.wasmExprCompiler.maybeTypeBoxingAndUnboxing(
                        stmt.returnExpression.exprType,
                        nearestFuncScope.funcType.returnType,
                        returnExprRef,
                    );
            }
        }
        const setReturnValue = module.local.set(
            this.currentFuncCtx.returnIdx,
            returnExprRef,
        );
        this.currentFuncCtx!.insert(setReturnValue);
        return brReturn;
    }

    WASMEmptyStmt(): binaryen.ExpressionRef {
        return this.WASMCompiler.module.nop();
    }

    WASMBaseLoopStmt(stmt: BaseLoopStatement): binaryen.ExpressionRef {
        const scope = stmt.getScope() as Scope;
        this.currentFuncCtx.enterScope(scope);

        let WASMCond: binaryen.ExpressionRef =
            this.WASMCompiler.wasmExpr.WASMExprGen(
                stmt.loopCondtion,
            ).binaryenRef;
        WASMCond = this.WASMCompiler.wasmExpr.generateCondition(WASMCond);
        const WASMStmts: binaryen.ExpressionRef = this.WASMStmtGen(
            stmt.loopBody,
        );
        // (block $break
        //  (loop $loop_label
        //   ...
        //   (if cond
        //    ...
        //    (br $loop_label)
        //   )
        //  )
        // )
        const flattenLoop: FlattenLoop = {
            label: stmt.loopLabel,
            condition: WASMCond,
            statements: WASMStmts,
        };

        this.currentFuncCtx.insert(
            this.WASMCompiler.module.loop(
                stmt.loopLabel,
                flattenLoopStatement(
                    flattenLoop,
                    stmt.statementKind,
                    this.WASMCompiler.module,
                ),
            ),
        );

        const statements = this.currentFuncCtx.exitScope();
        return this.WASMCompiler.module.block(stmt.loopBlockLabel, statements);
    }

    WASMForStmt(stmt: ForStatement): binaryen.ExpressionRef {
        const scope = stmt.getScope() as Scope;
        this.currentFuncCtx.enterScope(scope);

        let WASMCond: binaryen.ExpressionRef | undefined;
        let WASMIncrementor: binaryen.ExpressionRef | undefined;
        let WASMStmts: binaryen.ExpressionRef = this.WASMCompiler.module.nop();
        if (stmt.forLoopInitializer !== null) {
            const init = this.WASMStmtGen(stmt.forLoopInitializer);
            if (
                stmt.forLoopInitializer.statementKind ===
                ts.SyntaxKind.ExpressionStatement
            ) {
                this.currentFuncCtx.insert(init);
            }
        }
        if (stmt.forLoopCondtion !== null) {
            WASMCond = this.WASMCompiler.wasmExpr.WASMExprGen(
                stmt.forLoopCondtion,
            ).binaryenRef;
        }
        if (stmt.forLoopIncrementor !== null) {
            WASMIncrementor = this.WASMCompiler.wasmExpr.WASMExprGen(
                stmt.forLoopIncrementor,
            ).binaryenRef;
        }
        if (stmt.forLoopBody !== null) {
            WASMStmts = this.WASMStmtGen(stmt.forLoopBody);
        }
        const flattenLoop: FlattenLoop = {
            label: stmt.forLoopLabel,
            condition: WASMCond,
            statements: WASMStmts,
            incrementor: WASMIncrementor,
        };

        this.currentFuncCtx.insert(
            this.WASMCompiler.module.loop(
                stmt.forLoopLabel,
                flattenLoopStatement(
                    flattenLoop,
                    stmt.statementKind,
                    this.WASMCompiler.module,
                ),
            ),
        );

        const statements = this.currentFuncCtx.exitScope();
        return this.WASMCompiler.module.block(
            stmt.forLoopBlockLabel,
            statements,
        );
    }

    WASMSwitchStmt(stmt: SwitchStatement): binaryen.ExpressionRef {
        const WASMCond: binaryen.ExpressionRef =
            this.WASMCompiler.wasmExpr.WASMExprGen(
                stmt.switchCondition,
            ).binaryenRef;
        // switch
        //   |
        // CaseBlock
        //   |
        // [Clauses]
        return this.WASMCaseBlockStmt(
            <CaseBlock>stmt.switchCaseBlock,
            WASMCond,
        );
    }

    WASMCaseBlockStmt(
        stmt: CaseBlock,
        condtion: binaryen.ExpressionRef,
    ): binaryen.ExpressionRef {
        const clauses = stmt.caseCauses;
        if (clauses.length === 0) {
            return this.WASMCompiler.module.nop();
        }
        const module = this.WASMCompiler.module;
        const branches: binaryen.ExpressionRef[] = new Array(clauses.length);
        let indexOfDefault = -1;
        let idx = 0;
        clauses.forEach((clause, i) => {
            if (clause.statementKind === ts.SyntaxKind.DefaultClause) {
                indexOfDefault = i;
            } else {
                const caseCause = <CaseClause>clause;
                branches[idx++] = module.br(
                    'case' + i + stmt.switchLabel,
                    module.f64.eq(
                        condtion,
                        this.WASMCompiler.wasmExpr.WASMExprGen(
                            caseCause.caseExpr,
                        ).binaryenRef,
                    ),
                );
            }
        });
        const default_label =
            indexOfDefault === -1
                ? stmt.breakLabel
                : 'case' + indexOfDefault + stmt.switchLabel;
        branches[idx] = module.br(default_label);

        let block = module.block('case0' + stmt.switchLabel, branches);
        for (let i = 0; i !== clauses.length; ++i) {
            const clause = <CaseClause | DefaultClause>clauses[i];
            const label =
                i === clauses.length - 1
                    ? stmt.breakLabel
                    : 'case' + (i + 1) + stmt.switchLabel;
            block = module.block(
                label,
                [block].concat(this.WASMClauseStmt(clause)),
            );
        }

        return block;
    }

    WASMClauseStmt(clause: CaseClause | DefaultClause): binaryen.ExpressionRef {
        const scope = clause.getScope();
        this.currentFuncCtx.enterScope(scope!);

        for (const statement of clause.caseStatements) {
            this.currentFuncCtx.insert(this.WASMStmtGen(statement));
        }

        const statements = this.currentFuncCtx.exitScope();
        return this.WASMCompiler.module.block(null, statements);
    }

    WASMBreakStmt(stmt: BreakStatement): binaryen.ExpressionRef {
        return this.WASMCompiler.module.br(stmt.breakLabel);
    }

    WASMExpressionStmt(stmt: ExpressionStatement): binaryen.ExpressionRef {
        const innerExpr = stmt.expression;
        return this.WASMCompiler.wasmExpr.WASMExprGen(innerExpr).binaryenRef;
    }

    WASMVarStmt(stmt: VariableStatement): binaryen.ExpressionRef {
        const varArray = stmt.varArray;
        const module = this.WASMCompiler.module;
        const wasmType = this.WASMCompiler.wasmType;
        const wasmExpr = this.WASMCompiler.wasmExpr;
        const wasmDynExpr = this.WASMCompiler.wasmDynExprCompiler;
        const currentScope = this.currentFuncCtx.getCurrentScope();
        if (
            currentScope.kind !== ScopeKind.GlobalScope &&
            currentScope.kind !== ScopeKind.NamespaceScope
        ) {
            // common variable assignment
            for (const localVar of varArray) {
                if (localVar.initExpression !== null) {
                    let varInitExprRef: binaryen.ExpressionRef;
                    if (localVar.varType.kind === TypeKind.ANY) {
                        varInitExprRef = wasmDynExpr.WASMDynExprGen(
                            localVar.initExpression,
                        ).binaryenRef;
                        this.currentFuncCtx.insert(
                            module.local.set(localVar.varIndex, varInitExprRef),
                        );
                    } else {
                        varInitExprRef = wasmExpr.WASMExprGen(
                            localVar.initExpression,
                        ).binaryenRef;
                        // '||' token
                        if (
                            localVar.varType.kind === TypeKind.BOOLEAN &&
                            binaryen.getExpressionType(varInitExprRef) ===
                                binaryen.f64
                        ) {
                            const module = this.WASMCompiler.module;
                            varInitExprRef = module.i32.eqz(
                                module.i32.eqz(
                                    module.i32.trunc_u_sat.f64(varInitExprRef),
                                ),
                            );
                        }
                        /* In this version, we put free variable to context struct when parsing variable declaration */
                        if (localVar.varIsClosure) {
                            let scope = currentScope;
                            if (scope.kind !== ScopeKind.FunctionScope) {
                                scope = scope.getNearestFunctionScope()!;
                            }
                            const funcScope = <FunctionScope>scope;
                            /* free variable index in context struct */
                            const index = localVar.getClosureIndex();
                            const ctxStructType = <typeInfo>(
                                WASMGen.contextOfScope.get(funcScope)
                            );
                            const contextStruct = module.local.get(
                                (<ClosureEnvironment>scope).contextVariable!
                                    .varIndex,
                                ctxStructType.typeRef,
                            );
                            const freeVarSetWasmStmt =
                                binaryenCAPI._BinaryenStructSet(
                                    module.ptr,
                                    index,
                                    contextStruct,
                                    varInitExprRef,
                                );
                            this.currentFuncCtx.insert(freeVarSetWasmStmt);
                        } else {
                            if (
                                localVar.initExpression.exprType instanceof
                                    TSClass &&
                                localVar.varType instanceof TSClass
                            ) {
                                varInitExprRef =
                                    this.WASMCompiler.wasmExprCompiler.maybeTypeBoxingAndUnboxing(
                                        localVar.initExpression.exprType,
                                        localVar.varType,
                                        varInitExprRef,
                                    );
                            }
                            this.currentFuncCtx.insert(
                                module.local.set(
                                    localVar.varIndex,
                                    varInitExprRef,
                                ),
                            );
                        }
                    }
                }
            }
        } else {
            // add global variables
            for (const globalVar of varArray) {
                module.removeGlobal(globalVar.mangledName);
                const varTypeRef =
                    globalVar.varType.kind === TypeKind.FUNCTION
                        ? wasmType.getWASMFuncStructType(globalVar.varType)
                        : wasmType.getWASMType(globalVar.varType);
                if (globalVar.isDeclare()) {
                    module.addGlobalImport(
                        globalVar.mangledName,
                        BuiltinNames.externalModuleName,
                        globalVar.varName,
                        varTypeRef,
                    );
                    continue;
                }
                const mutable = !globalVar.isConst();
                if (globalVar.initExpression === null) {
                    module.addGlobal(
                        globalVar.mangledName,
                        varTypeRef,
                        mutable,
                        this.WASMCompiler.getVariableInitValue(
                            globalVar.varType,
                        ),
                    );
                } else {
                    const varInitExprRef = wasmExpr.WASMExprGen(
                        globalVar.initExpression,
                    ).binaryenRef;
                    if (globalVar.varType.kind === TypeKind.NUMBER) {
                        if (
                            globalVar.initExpression.expressionKind ===
                            ts.SyntaxKind.NumericLiteral
                        ) {
                            module.addGlobal(
                                globalVar.mangledName,
                                varTypeRef,
                                mutable,
                                varInitExprRef,
                            );
                        } else {
                            module.addGlobal(
                                globalVar.mangledName,
                                varTypeRef,
                                true,
                                globalVar.varType.kind === TypeKind.NUMBER
                                    ? module.f64.const(0)
                                    : module.i64.const(0, 0),
                            );
                            this.currentFuncCtx!.insert(
                                module.global.set(
                                    globalVar.mangledName,
                                    varInitExprRef,
                                ),
                            );
                        }
                    } else if (globalVar.varType.kind === TypeKind.BOOLEAN) {
                        module.addGlobal(
                            globalVar.mangledName,
                            varTypeRef,
                            mutable,
                            varInitExprRef,
                        );
                    } else {
                        module.addGlobal(
                            globalVar.mangledName,
                            varTypeRef,
                            true,
                            binaryenCAPI._BinaryenRefNull(
                                module.ptr,
                                varTypeRef,
                            ),
                        );
                        if (globalVar.varType.kind === TypeKind.ANY) {
                            const dynInitExprRef =
                                this.WASMCompiler.wasmDynExprCompiler.WASMDynExprGen(
                                    globalVar.initExpression,
                                ).binaryenRef;
                            this.currentFuncCtx!.insert(
                                module.global.set(
                                    globalVar.mangledName,
                                    dynInitExprRef,
                                ),
                            );
                        } else {
                            this.currentFuncCtx!.insert(
                                module.global.set(
                                    globalVar.mangledName,
                                    varInitExprRef,
                                ),
                            );
                        }
                    }
                }
            }
        }
        return module.unreachable();
    }

    WASMImportStmt(stmt: ImportDeclaration): binaryen.ExpressionRef {
        const module = this.WASMCompiler.module;
        /** Currently, we put all ts files into a whole wasm file.
         *  So we don't need to addGlobalImport and addFunctionImport here.
         *  If we generate several wasm files, we need to add imports here.
         */
        if (stmt.importModuleStartFuncName !== '') {
            return module.call(
                stmt.importModuleStartFuncName,
                [],
                binaryen.none,
            );
        }
        return module.unreachable();
    }
}
