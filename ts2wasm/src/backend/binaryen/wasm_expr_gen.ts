/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import ts from 'typescript';
import binaryen from 'binaryen';
import * as binaryenCAPI from './glue/binaryen.js';
import {
    builtinTypes,
    FunctionKind,
    TSArray,
    TSClass,
    TSFunction,
    TSInterface,
    Type,
    TypeKind,
} from '../../type.js';
import { Variable } from '../../variable.js';
import {
    BinaryExpression,
    CallExpression,
    ConditionalExpression,
    Expression,
    IdentifierExpression,
    NewExpression,
    NumberLiteralExpression,
    StringLiteralExpression,
    SuperCallExpression,
    UnaryExpression,
    ArrayLiteralExpression,
    ObjectLiteralExpression,
    PropertyAccessExpression,
    ElementAccessExpression,
    AsExpression,
    ParenthesizedExpression,
    FunctionExpression,
} from '../../expression.js';
import {
    arrayToPtr,
    createCondBlock,
    emptyStructType,
} from './glue/transform.js';
import { assert } from 'console';
import {
    FunctionScope,
    GlobalScope,
    ClassScope,
    ScopeKind,
    Scope,
    NamespaceScope,
    ClosureEnvironment,
} from '../../scope.js';
import { MatchKind, Stack } from '../../utils.js';
import { dyntype, structdyn } from './lib/dyntype/utils.js';
import { BuiltinNames } from '../../../lib/builtin/builtin_name.js';
import { charArrayTypeInfo, stringTypeInfo } from './glue/packType.js';
import { WASMGen } from './index.js';
import { Logger } from '../../log.js';
import { getClassNameByTypeKind, unboxAnyTypeToBaseType } from './utils.js';

export interface WasmValue {
    /* binaryen reference */
    binaryenRef: binaryen.ExpressionRef;
    /* original type in source code */
    tsType: Type;
}

/* Access information, this is used as return value of IdentifierExpr handler
    or PropertyAccessExpr handler. If these handlers are called with byRef == true,
    or the result is not a direct value (Type, scope), then the Access information
    is returned
*/

enum AccessType {
    LocalVar,
    GlobalVar,
    ClosureVar,
    Function,
    Method,
    Getter,
    Setter,
    Struct,
    Interface,
    Array,
    DynObject,
    DynArray,
    Type,
    Scope,
}

class AccessBase {
    constructor(public readonly accessType: AccessType) {}
}

class TypedAccessBase extends AccessBase {
    constructor(
        public readonly accessType: AccessType,
        public readonly tsType: Type,
    ) {
        super(accessType);
    }
}

class LocalAccess extends TypedAccessBase {
    constructor(
        public index: number,
        public wasmType: binaryenCAPI.TypeRef,
        public tsType: Type,
    ) {
        super(AccessType.LocalVar, tsType);
    }
}

class GlobalAccess extends TypedAccessBase {
    constructor(
        public varName: string,
        public wasmType: binaryenCAPI.TypeRef,
        public tsType: Type,
    ) {
        super(AccessType.GlobalVar, tsType);
    }
}

class FunctionAccess extends AccessBase {
    constructor(public funcScope: FunctionScope) {
        super(AccessType.Function);
    }
}

class MethodAccess extends AccessBase {
    public mangledMethodName;
    constructor(
        public methodType: TSFunction,
        public methodIndex: number,
        public classType: TSClass,
        public thisObj: binaryen.ExpressionRef | null = null,
        public isBuiltInMethod: boolean = false,
        public methodName: string = '',
    ) {
        super(AccessType.Function);
        this.mangledMethodName = classType.mangledName.concat(
            BuiltinNames.moduleDelimiter,
            methodName,
        );
    }
}

class InfcMethodAccess extends AccessBase {
    constructor(
        public infcTypeId: binaryen.ExpressionRef,
        public objTypeId: binaryen.ExpressionRef,
        public objRef: binaryen.ExpressionRef,
        public objType: binaryenCAPI.TypeRef, // ref.cast objHeapType anyref
        public methodIndex: number,
        public dynMethodIndex: binaryen.ExpressionRef,
        public infcType: TSInterface,
        public methodType: TSFunction,
    ) {
        super(AccessType.Function);
    }
}

class GetterAccess extends AccessBase {
    constructor(
        public methodType: TSFunction,
        public methodIndex: number,
        public classType: TSClass,
        public thisObj: binaryen.ExpressionRef,
    ) {
        super(AccessType.Getter);
    }
}

class InfcGetterAccess extends AccessBase {
    constructor(
        public infcTypeId: binaryen.ExpressionRef,
        public objTypeId: binaryen.ExpressionRef,
        public objRef: binaryen.ExpressionRef,
        public objType: binaryenCAPI.TypeRef, // ref.cast objHeapType anyref
        public methodIndex: number,
        public dynMethodIndex: binaryen.ExpressionRef,
        public infcType: TSInterface,
        public methodType: TSFunction,
    ) {
        super(AccessType.Getter);
    }
}

class StructAccess extends TypedAccessBase {
    constructor(
        public ref: binaryen.ExpressionRef,
        public fieldIndex: number,
        public wasmType: binaryenCAPI.TypeRef,
        tsType: Type,
    ) {
        super(AccessType.Struct, tsType);
    }
}

class InterfaceAccess extends TypedAccessBase {
    constructor(
        public infcTypeId: binaryen.ExpressionRef,
        public objTypeId: binaryen.ExpressionRef,
        public objRef: binaryen.ExpressionRef,
        public objType: binaryenCAPI.TypeRef,
        public fieldIndex: number,
        public dynFieldIndex: binaryen.ExpressionRef,
        tsType: Type,
    ) {
        super(AccessType.Interface, tsType);
    }
}

class ArrayAccess extends TypedAccessBase {
    constructor(
        public ref: binaryen.ExpressionRef,
        public index: number,
        public wasmType: binaryenCAPI.TypeRef,
        tsType: Type,
    ) {
        super(AccessType.Array, tsType);
    }
}

class DynObjectAccess extends AccessBase {
    constructor(public ref: binaryen.ExpressionRef, public fieldName: string) {
        super(AccessType.DynObject);
    }
}

class DynArrayAccess extends AccessBase {
    constructor(
        public ref: binaryen.ExpressionRef,
        public index: binaryen.ExpressionRef,
    ) {
        super(AccessType.DynArray);
    }
}

class TypeAccess extends AccessBase {
    constructor(public type: Type) {
        super(AccessType.Type);
    }
}

class ScopeAccess extends AccessBase {
    constructor(public scope: Scope) {
        super(AccessType.Scope);
    }
}

export class WASMExpressionBase {
    wasmCompiler;
    module;
    wasmType;
    currentFuncCtx;
    globalTmpVarStack;
    localTmpVarStack;
    staticValueGen;
    dynValueGen;
    enterModuleScope;
    curExtrefTableIdx = -1;
    extrefTableSize = 0;

    constructor(WASMCompiler: WASMGen) {
        this.wasmCompiler = WASMCompiler;
        this.module = this.wasmCompiler.module;
        this.wasmType = this.wasmCompiler.wasmType;
        this.currentFuncCtx = this.wasmCompiler.curFunctionCtx!;
        this.globalTmpVarStack = new Stack<string>();
        this.localTmpVarStack = new Stack<string>();
        this.staticValueGen = this.wasmCompiler.wasmExprCompiler;
        this.dynValueGen = this.wasmCompiler.wasmDynExprCompiler;
        this.enterModuleScope = this.wasmCompiler.enterModuleScope;
    }

    setLocalValue(
        variableIndex: number,
        value: binaryen.ExpressionRef,
    ): binaryen.ExpressionRef {
        return this.module.local.set(variableIndex, value);
    }

    getLocalValue(
        variableIndex: number,
        variableType: binaryen.Type,
    ): binaryen.ExpressionRef {
        return this.module.local.get(variableIndex, variableType);
    }

    setGlobalValue(
        variableName: string,
        value: binaryen.ExpressionRef,
    ): binaryen.ExpressionRef {
        return this.module.global.set(variableName, value);
    }

    getGlobalValue(
        variableName: string,
        variableType: binaryen.Type,
    ): binaryen.ExpressionRef {
        return this.module.global.get(variableName, variableType);
    }

    generateTmpVar(prefix: string, typeName = '', varType = new Type()) {
        // add tmp value to current scope
        const tmpNumberName = this.getTmpVariableName(prefix);
        let variableType;
        if (typeName === 'any') {
            variableType = builtinTypes.get(TypeKind.ANY)!;
        } else if (typeName === 'address') {
            variableType = builtinTypes.get(TypeKind.BOOLEAN)!;
        } else if (typeName === 'number') {
            variableType = builtinTypes.get(TypeKind.NUMBER)!;
        } else if (typeName === 'boolean') {
            variableType = builtinTypes.get(TypeKind.BOOLEAN)!;
        } else {
            variableType = varType;
        }
        const tmpVar = new Variable(tmpNumberName, variableType, [], -1, true);
        this.addVariableToCurrentScope(tmpVar);
        return tmpVar;
    }

    getTmpVariableName(prefix: string) {
        const currentScope = this.currentFuncCtx.getCurrentScope();
        let tmpVariableName: string;
        if (currentScope.kind === ScopeKind.GlobalScope) {
            tmpVariableName = prefix + this.globalTmpVarStack.size();
            this.globalTmpVarStack.push(tmpVariableName);
        } else {
            tmpVariableName = prefix + this.localTmpVarStack.size();
            this.localTmpVarStack.push(tmpVariableName);
        }
        return tmpVariableName;
    }

    addVariableToCurrentScope(variable: Variable) {
        const currentScope = this.currentFuncCtx.getCurrentScope();
        let targetScope: Scope | null = currentScope.getNearestFunctionScope();
        if (!targetScope) {
            targetScope = currentScope.getRootGloablScope()!;
        }

        const variableIndex = targetScope.allocateLocalIndex();
        variable.setVarIndex(variableIndex);
        targetScope.addTempVar(variable);
    }

    setVariableToCurrentScope(
        variable: Variable,
        value: binaryen.ExpressionRef,
    ): binaryen.ExpressionRef {
        return this.module.local.set(variable.varIndex, value);
    }

    getVariableValue(variable: Variable, type: binaryen.Type) {
        return this.getLocalValue(variable.varIndex, type);
    }

    convertTypeToI32(
        expression: binaryen.ExpressionRef,
        expressionType: binaryen.Type,
    ): binaryen.ExpressionRef {
        const module = this.module;
        switch (expressionType) {
            case binaryen.f64: {
                return module.i32.trunc_u_sat.f64(expression);
            }
            case binaryen.i32: {
                return expression;
            }
            // TODO: deal with more types
        }
        return binaryen.none;
    }

    convertTypeToI64(
        expression: binaryen.ExpressionRef,
        expressionType: binaryen.Type,
    ): binaryen.ExpressionRef {
        const module = this.module;
        switch (expressionType) {
            case binaryen.f64: {
                return module.i64.trunc_u_sat.f64(expression);
            }
            case binaryen.i64: {
                return expression;
            }
            // TODO: deal with more types
        }
        return binaryen.none;
    }

    convertTypeToF64(
        expression: binaryen.ExpressionRef,
        expressionType: binaryen.Type,
    ): binaryen.ExpressionRef {
        const module = this.module;
        switch (expressionType) {
            case binaryen.i32: {
                return module.f64.convert_u.i32(expression);
            }
            case binaryen.i64: {
                return module.f64.convert_u.i64(expression);
            }
            // TODO: deal with more types
        }
        return binaryen.none;
    }

    unboxAnyToBase(anyExprRef: binaryen.ExpressionRef, typeKind: TypeKind) {
        return unboxAnyTypeToBaseType(this.module, anyExprRef, typeKind);
    }

    unboxAnyToExtref(anyExprRef: binaryen.ExpressionRef, targetType: Type) {
        const module = this.module;

        const isExternRef = module.call(
            dyntype.dyntype_is_extref,
            [
                module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t),
                anyExprRef,
            ],
            dyntype.bool,
        );
        const condition = module.i32.eq(isExternRef, module.i32.const(1));
        const wasmType = this.wasmType.getWASMType(targetType);
        // iff True
        const tableIndex = module.call(
            dyntype.dyntype_to_extref,
            [
                module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t),
                anyExprRef,
            ],
            dyntype.int,
        );
        const externalRef = module.table.get(
            BuiltinNames.extrefTable,
            tableIndex,
            binaryen.anyref,
        );
        const value = binaryenCAPI._BinaryenRefCast(
            module.ptr,
            externalRef,
            wasmType,
        );
        // iff False
        const unreachableRef = module.unreachable();

        const blockStmt = module.if(condition, value, unreachableRef);
        return module.block(null, [blockStmt], wasmType);
    }

    boxBaseTypeToAny(expr: Expression): binaryen.ExpressionRef {
        let res: binaryen.ExpressionRef;
        const staticRef = this.staticValueGen.WASMExprGen(expr).binaryenRef;
        switch (expr.exprType.kind) {
            case TypeKind.NUMBER:
                res = this.generateDynNumber(staticRef);
                break;
            case TypeKind.BOOLEAN:
                res = this.generateDynBoolean(staticRef);
                break;
            case TypeKind.STRING: {
                /** TODO: need to do more research on string */
                res = this.generateDynString(staticRef);
                break;
            }
            case TypeKind.NULL:
                res = this.generateDynNull();
                break;
            default:
                throw Error(
                    `unboxing static type to any type, unsupported static type : ${expr.exprType.kind}`,
                );
        }
        return res;
    }

    boxNonLiteralToAny(expr: Expression): binaryen.ExpressionRef {
        let res: binaryen.ExpressionRef;
        if (
            expr instanceof IdentifierExpression &&
            expr.identifierName === 'undefined'
        ) {
            res = this.generateDynUndefined();
        } else {
            /** box non-literal expression to any:
             *  new dynamic value: number, boolean, null
             *  direct assignment: any
             *  new string: string (which will be put into table too)
             *  new extref: obj (including class type, interface type, array type)
             */
            const staticRef = this.staticValueGen.WASMExprGen(expr).binaryenRef;
            switch (expr.exprType.kind) {
                case TypeKind.NUMBER:
                case TypeKind.BOOLEAN:
                case TypeKind.STRING:
                case TypeKind.NULL:
                    res = this.boxBaseTypeToAny(expr);
                    break;
                case TypeKind.ANY:
                    res = staticRef;
                    break;
                // case TypeKind.STRING:
                case TypeKind.INTERFACE:
                case TypeKind.ARRAY:
                case TypeKind.CLASS:
                case TypeKind.FUNCTION:
                    res = this.generateDynExtref(staticRef, expr.exprType.kind);
                    break;
                default:
                    throw Error(
                        `boxing static type to any type failed, static type is: ${expr.exprType.kind}`,
                    );
            }
        }
        return res;
    }

    operateF64F64(
        leftExprRef: binaryen.ExpressionRef,
        rightExprRef: binaryen.ExpressionRef,
        operatorKind: ts.SyntaxKind,
    ) {
        const module = this.module;
        switch (operatorKind) {
            case ts.SyntaxKind.PlusToken: {
                return module.f64.add(leftExprRef, rightExprRef);
            }
            case ts.SyntaxKind.MinusToken: {
                return module.f64.sub(leftExprRef, rightExprRef);
            }
            case ts.SyntaxKind.AsteriskToken: {
                return module.f64.mul(leftExprRef, rightExprRef);
            }
            case ts.SyntaxKind.SlashToken: {
                return module.f64.div(leftExprRef, rightExprRef);
            }
            case ts.SyntaxKind.GreaterThanToken: {
                return module.f64.gt(leftExprRef, rightExprRef);
            }
            case ts.SyntaxKind.GreaterThanEqualsToken: {
                return module.f64.ge(leftExprRef, rightExprRef);
            }
            case ts.SyntaxKind.LessThanToken: {
                return module.f64.lt(leftExprRef, rightExprRef);
            }
            case ts.SyntaxKind.LessThanEqualsToken: {
                return module.f64.le(leftExprRef, rightExprRef);
            }
            case ts.SyntaxKind.LessThanLessThanToken: {
                return this.convertTypeToF64(
                    module.i64.shl(
                        this.convertTypeToI64(leftExprRef, binaryen.f64),
                        this.convertTypeToI64(rightExprRef, binaryen.f64),
                    ),
                    binaryen.i64,
                );
            }
            case ts.SyntaxKind.EqualsEqualsToken:
            case ts.SyntaxKind.EqualsEqualsEqualsToken: {
                return module.f64.eq(leftExprRef, rightExprRef);
            }
            case ts.SyntaxKind.ExclamationEqualsToken:
            case ts.SyntaxKind.ExclamationEqualsEqualsToken: {
                return module.f64.ne(leftExprRef, rightExprRef);
            }
            case ts.SyntaxKind.AmpersandAmpersandToken: {
                return module.select(
                    this.convertTypeToI32(leftExprRef, binaryen.f64),
                    rightExprRef,
                    leftExprRef,
                    binaryen.f64,
                );
            }
            case ts.SyntaxKind.BarBarToken: {
                return module.select(
                    this.convertTypeToI32(leftExprRef, binaryen.f64),
                    leftExprRef,
                    rightExprRef,
                    binaryen.f64,
                );
            }
            case ts.SyntaxKind.AmpersandToken: {
                return this.convertTypeToF64(
                    module.i64.and(
                        this.convertTypeToI64(leftExprRef, binaryen.f64),
                        this.convertTypeToI64(rightExprRef, binaryen.f64),
                    ),
                    binaryen.i64,
                );
            }
            case ts.SyntaxKind.BarToken: {
                return this.convertTypeToF64(
                    module.i64.or(
                        this.convertTypeToI64(leftExprRef, binaryen.f64),
                        this.convertTypeToI64(rightExprRef, binaryen.f64),
                    ),
                    binaryen.i64,
                );
            }
            default:
                return module.unreachable();
        }
    }

    operateF64I32(
        leftExprRef: binaryen.ExpressionRef,
        rightExprRef: binaryen.ExpressionRef,
        operatorKind: ts.SyntaxKind,
    ) {
        const module = this.module;
        switch (operatorKind) {
            case ts.SyntaxKind.AmpersandAmpersandToken: {
                return module.select(
                    this.convertTypeToI32(leftExprRef, binaryen.f64),
                    rightExprRef,
                    this.convertTypeToI32(leftExprRef, binaryen.f64),
                    binaryen.i32,
                );
            }
            case ts.SyntaxKind.BarBarToken: {
                return module.select(
                    this.convertTypeToI32(leftExprRef, binaryen.f64),
                    leftExprRef,
                    this.convertTypeToF64(rightExprRef, binaryen.i32),
                    binaryen.f64,
                );
            }
            default:
                return module.unreachable();
        }
    }

    operateI32F64(
        leftExprRef: binaryen.ExpressionRef,
        rightExprRef: binaryen.ExpressionRef,
        operatorKind: ts.SyntaxKind,
    ) {
        const module = this.module;
        switch (operatorKind) {
            case ts.SyntaxKind.AmpersandAmpersandToken: {
                const condition = Boolean(module.i32.eqz(leftExprRef));
                if (condition) {
                    return module.select(
                        leftExprRef,
                        this.convertTypeToI32(rightExprRef, binaryen.f64),
                        leftExprRef,
                        binaryen.i32,
                    );
                } else {
                    return rightExprRef;
                }
            }
            case ts.SyntaxKind.BarBarToken: {
                // if left is false, then condition is true
                const condition = Boolean(module.i32.eqz(leftExprRef));
                if (condition) {
                    return rightExprRef;
                } else {
                    return module.select(
                        leftExprRef,
                        this.convertTypeToF64(leftExprRef, binaryen.i32),
                        rightExprRef,
                        binaryen.f64,
                    );
                }
            }
            default:
                return module.unreachable();
        }
    }

    operateI32I32(
        leftExprRef: binaryen.ExpressionRef,
        rightExprRef: binaryen.ExpressionRef,
        operatorKind: ts.SyntaxKind,
    ) {
        const module = this.module;
        switch (operatorKind) {
            case ts.SyntaxKind.AmpersandAmpersandToken: {
                return module.select(
                    leftExprRef,
                    rightExprRef,
                    leftExprRef,
                    binaryen.i32,
                );
            }
            case ts.SyntaxKind.BarBarToken: {
                return module.select(
                    leftExprRef,
                    leftExprRef,
                    rightExprRef,
                    binaryen.i32,
                );
            }
            default:
                return module.unreachable();
        }
    }

    operateAnyAny(
        leftExprRef: binaryen.ExpressionRef,
        rightExprRef: binaryen.ExpressionRef,
        operatorKind: ts.SyntaxKind,
    ) {
        const tmpLeftNumberRef = this.unboxAnyToBase(
            leftExprRef,
            TypeKind.NUMBER,
        );
        const tmpRightNumberRef = this.unboxAnyToBase(
            rightExprRef,
            TypeKind.NUMBER,
        );
        const tmpTotalNumberName = this.getTmpVariableName('~numberTotal|');
        const tmpTotalNumberVar: Variable = new Variable(
            tmpTotalNumberName,
            builtinTypes.get(TypeKind.ANY)!,
            [],
            0,
        );
        const setTotalNumberExpression = this.oprateF64F64ToDyn(
            tmpLeftNumberRef,
            tmpRightNumberRef,
            operatorKind,
            tmpTotalNumberVar,
        );
        // store the external operations into currentScope's statementArray
        this.currentFuncCtx.insert(setTotalNumberExpression);
        return this.getVariableValue(tmpTotalNumberVar, binaryen.anyref);
    }

    operateAnyNumber(
        leftExprRef: binaryen.ExpressionRef,
        rightExprRef: binaryen.ExpressionRef,
        operatorKind: ts.SyntaxKind,
    ) {
        const tmpLeftNumberRef = this.unboxAnyToBase(
            leftExprRef,
            TypeKind.NUMBER,
        );
        const tmpTotalNumberName = this.getTmpVariableName('~numberTotal|');
        const tmpTotalNumberVar: Variable = new Variable(
            tmpTotalNumberName,
            builtinTypes.get(TypeKind.ANY)!,
            [],
            0,
        );
        const setTotalNumberExpression = this.oprateF64F64ToDyn(
            tmpLeftNumberRef,
            rightExprRef,
            operatorKind,
            tmpTotalNumberVar,
        );
        // store the external operations into currentScope's statementArray
        this.currentFuncCtx.insert(setTotalNumberExpression);
        return this.getVariableValue(tmpTotalNumberVar, binaryen.anyref);
    }

    oprateF64F64ToDyn(
        leftNumberExpression: binaryen.ExpressionRef,
        rightNumberExpression: binaryen.ExpressionRef,
        operatorKind: ts.SyntaxKind,
        tmpTotalNumberVar: Variable,
    ) {
        // operate left expression and right expression
        const operateTotalNumber = this.operateF64F64(
            leftNumberExpression,
            rightNumberExpression,
            operatorKind,
        );
        // add tmp total number value to current scope
        this.addVariableToCurrentScope(tmpTotalNumberVar);
        const setTotalNumberExpression = this.setVariableToCurrentScope(
            tmpTotalNumberVar,
            this.generateDynNumber(operateTotalNumber),
        );
        return setTotalNumberExpression;
    }

    defaultValue(typeKind: TypeKind) {
        switch (typeKind) {
            case TypeKind.BOOLEAN:
                return this.module.i32.const(0);
            case TypeKind.NUMBER:
                return this.module.f64.const(0);
            case TypeKind.STRING:
                return binaryenCAPI._BinaryenRefNull(
                    this.module.ptr,
                    binaryenCAPI._BinaryenTypeStructref(),
                );
            default:
                // TODO
                return binaryen.none;
        }
    }

    generateStringRef(value: string) {
        const valueLen = value.length;
        let strRelLen = valueLen;
        const charArray = [];
        for (let i = 0; i < valueLen; i++) {
            const codePoint = value.codePointAt(i)!;
            if (codePoint > 0xffff) {
                i++;
                strRelLen--;
            }
            charArray.push(this.module.i32.const(codePoint));
        }
        const valueContent = binaryenCAPI._BinaryenArrayInit(
            this.module.ptr,
            charArrayTypeInfo.heapTypeRef,
            arrayToPtr(charArray).ptr,
            strRelLen,
        );
        const wasmStringValue = binaryenCAPI._BinaryenStructNew(
            this.module.ptr,
            arrayToPtr([this.module.i32.const(0), valueContent]).ptr,
            2,
            stringTypeInfo.heapTypeRef,
        );
        return wasmStringValue;
    }

    generateDynNumber(dynValue: binaryen.ExpressionRef) {
        const module = this.module;
        return module.call(
            dyntype.dyntype_new_number,
            [
                module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t),
                dynValue,
            ],
            dyntype.dyn_value_t,
        );
    }

    generateDynBoolean(dynValue: binaryen.ExpressionRef) {
        const module = this.module;
        return module.call(
            dyntype.dyntype_new_boolean,
            [
                module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t),
                dynValue,
            ],
            dyntype.dyn_value_t,
        );
    }

    generateDynString(dynValue: binaryen.ExpressionRef) {
        const module = this.module;
        return module.call(
            dyntype.dyntype_new_string,
            [
                module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t),
                dynValue,
            ],
            dyntype.dyn_value_t,
        );
    }

    generateDynNull() {
        const module = this.module;
        return module.call(
            dyntype.dyntype_new_null,
            [module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t)],
            dyntype.dyn_value_t,
        );
    }

    generateDynUndefined() {
        const module = this.module;
        return module.call(
            dyntype.dyntype_new_undefined,
            [module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t)],
            dyntype.dyn_value_t,
        );
    }

    generateDynArray() {
        const module = this.module;
        return module.call(
            dyntype.dyntype_new_array,
            [module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t)],
            dyntype.dyn_value_t,
        );
    }

    generateDynObj() {
        const module = this.module;
        return module.call(
            dyntype.dyntype_new_object,
            [module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t)],
            dyntype.dyn_value_t,
        );
    }

    generateDynExtref(
        dynValue: binaryen.ExpressionRef,
        extrefTypeKind: TypeKind,
    ) {
        const module = this.module;
        // table type is anyref, no need to cast
        /** we regard string-nonLiteral as extref too */
        let dynFuncName: string;
        let extObjKind: dyntype.ExtObjKind = 0;
        switch (extrefTypeKind) {
            // case TypeKind.STRING: {
            //     dynFuncName = dyntype.dyntype_new_string;
            //     break;
            // }
            case TypeKind.CLASS: {
                dynFuncName = dyntype.dyntype_new_extref;
                extObjKind = dyntype.ExtObjKind.ExtObj;
                break;
            }
            case TypeKind.FUNCTION: {
                dynFuncName = dyntype.dyntype_new_extref;
                extObjKind = dyntype.ExtObjKind.ExtFunc;
                break;
            }
            case TypeKind.INTERFACE: {
                dynFuncName = dyntype.dyntype_new_extref;
                extObjKind = dyntype.ExtObjKind.ExtInfc;
                break;
            }
            case TypeKind.ARRAY: {
                dynFuncName = dyntype.dyntype_new_extref;
                extObjKind = dyntype.ExtObjKind.ExtArray;
                break;
            }
            default: {
                throw Error(
                    `unexpected type kind when boxing to external reference, type kind is ${extrefTypeKind}`,
                );
            }
        }
        if (++this.curExtrefTableIdx >= this.extrefTableSize) {
            const tableGrowExpr = module.table.grow(
                BuiltinNames.extrefTable,
                binaryenCAPI._BinaryenRefNull(
                    this.module.ptr,
                    binaryenCAPI._BinaryenTypeStructref(),
                ),
                module.i32.const(BuiltinNames.tableGrowDelta),
            );
            this.extrefTableSize += BuiltinNames.tableGrowDelta;
            this.currentFuncCtx.insert(module.drop(tableGrowExpr));
        }
        const tableSetOp = module.table.set(
            BuiltinNames.extrefTable,
            module.i32.const(this.curExtrefTableIdx),
            dynValue,
        );
        this.currentFuncCtx.insert(tableSetOp);
        return module.call(
            dynFuncName,
            [
                module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t),
                module.i32.const(this.curExtrefTableIdx),
                module.i32.const(extObjKind),
            ],
            dyntype.dyn_value_t,
        );
    }

    getArrayInitFromArrayType(arrayType: TSArray): binaryen.ExpressionRef {
        const module = this.module;
        const elemType = arrayType.elementType;
        switch (elemType.kind) {
            case TypeKind.NUMBER: {
                return module.f64.const(0);
            }
            case TypeKind.STRING: {
                return this.generateStringRef('');
            }
            case TypeKind.BOOLEAN: {
                return module.i32.const(0);
            }
            default: {
                return binaryenCAPI._BinaryenRefNull(
                    module.ptr,
                    this.wasmType.getWASMType(elemType),
                );
            }
        }
    }

    generateCondition(exprRef: binaryen.ExpressionRef) {
        const type = binaryen.getExpressionType(exprRef);
        // const module = this.WASMCompiler.module;
        let res = this.module.unreachable();
        /* TODO: Haven't handle string yet */
        if (type === binaryen.i32) {
            res = exprRef;
        } else if (type === binaryen.f64) {
            res = this.module.f64.ne(exprRef, this.module.f64.const(0));
        } else {
            res = this.module.i32.eqz(
                binaryenCAPI._BinaryenRefIsNull(this.module.ptr, exprRef),
            );
        }
        return res;
    }
}

export class WASMExpressionGen extends WASMExpressionBase {
    constructor(WASMCompiler: WASMGen) {
        super(WASMCompiler);
    }

    WASMExprGen(expr: Expression): WasmValue {
        const res = this.WASMExprGenInternal(expr);
        if (res instanceof AccessBase) {
            throw Error(`Expression is not a value`);
        }
        return res as WasmValue;
    }

    private WASMExprGenInternal(
        expr: Expression,
        byRef = false,
    ): WasmValue | AccessBase {
        this.module = this.wasmCompiler.module;
        this.wasmType = this.wasmCompiler.wasmType;
        this.staticValueGen = this.wasmCompiler.wasmExprCompiler;
        this.dynValueGen = this.wasmCompiler.wasmDynExprCompiler;
        this.currentFuncCtx = this.wasmCompiler.curFunctionCtx!;
        this.enterModuleScope = this.wasmCompiler.enterModuleScope!;

        let res: binaryen.ExpressionRef | AccessBase;

        switch (expr.expressionKind) {
            case ts.SyntaxKind.NumericLiteral:
                res = this.WASMNumberLiteral(<NumberLiteralExpression>expr);
                break;
            case ts.SyntaxKind.FalseKeyword:
                res = this.module.i32.const(0);
                break;
            case ts.SyntaxKind.TrueKeyword:
                res = this.module.i32.const(1);
                break;
            case ts.SyntaxKind.NullKeyword:
                res = this.module.ref.null(
                    binaryenCAPI._BinaryenTypeStructref(),
                );
                break;
            case ts.SyntaxKind.StringLiteral:
                res = this.WASMStringLiteral(<StringLiteralExpression>expr);
                break;
            case ts.SyntaxKind.Identifier:
                res = this.WASMIdenfierExpr(<IdentifierExpression>expr, byRef);
                break;
            case ts.SyntaxKind.BinaryExpression:
                res = this.WASMBinaryExpr(<BinaryExpression>expr);
                break;
            case ts.SyntaxKind.PrefixUnaryExpression:
            case ts.SyntaxKind.PostfixUnaryExpression:
                res = this.WASMUnaryExpr(<UnaryExpression>expr);
                break;
            case ts.SyntaxKind.ConditionalExpression:
                res = this.WASMConditionalExpr(<ConditionalExpression>expr);
                break;
            case ts.SyntaxKind.CallExpression: {
                res = this.WASMCallExpr(<CallExpression>expr);
                break;
            }
            case ts.SyntaxKind.SuperKeyword: {
                res = this.WASMSuperExpr(<SuperCallExpression>expr);
                break;
            }
            case ts.SyntaxKind.ParenthesizedExpression: {
                const parentesizedExpr = <ParenthesizedExpression>expr;
                return this.WASMExprGenInternal(
                    parentesizedExpr.parentesizedExpr,
                );
            }
            case ts.SyntaxKind.ArrayLiteralExpression:
                res = this.WASMArrayLiteralExpr(<ArrayLiteralExpression>expr);
                break;
            case ts.SyntaxKind.ObjectLiteralExpression:
                res = this.WASMObjectLiteralExpr(<ObjectLiteralExpression>expr);
                break;
            case ts.SyntaxKind.PropertyAccessExpression:
                res = this.WASMPropertyAccessExpr(
                    <PropertyAccessExpression>expr,
                    byRef,
                );
                break;
            case ts.SyntaxKind.ElementAccessExpression:
                res = this.WASMElementAccessExpr(
                    <ElementAccessExpression>expr,
                    byRef,
                );
                break;
            case ts.SyntaxKind.NewExpression: {
                res = this.WASMNewExpr(<NewExpression>expr);
                break;
            }
            case ts.SyntaxKind.AsExpression:
                res = this.WASMAsExpr(<AsExpression>expr);
                break;
            case ts.SyntaxKind.FunctionExpression:
            case ts.SyntaxKind.ArrowFunction:
                res = this.WASMFuncExpr(<FunctionExpression>expr);
                break;
            default:
                throw new Error('unexpected expr kind ' + expr.expressionKind);
        }

        if (res instanceof AccessBase) {
            return res;
        } else {
            return {
                binaryenRef: res,
                tsType: expr.exprType,
            };
        }
    }

    private WASMNumberLiteral(
        expr: NumberLiteralExpression,
    ): binaryen.ExpressionRef {
        return this.module.f64.const(expr.expressionValue);
    }

    private WASMStringLiteral(
        expr: StringLiteralExpression,
    ): binaryen.ExpressionRef {
        const value = expr.expressionValue.substring(
            1,
            expr.expressionValue.length - 1,
        );
        return this.generateStringRef(value);
    }

    private _loadFromAccessInfo(
        accessInfo: AccessBase,
    ): binaryen.ExpressionRef | AccessBase {
        const module = this.module;
        let loadRef: binaryen.ExpressionRef = 0;

        /* Load value according to accessInfo returned from
            Identifier or PropertyAccess */
        if (accessInfo instanceof GlobalAccess) {
            const { varName, wasmType } = accessInfo;
            loadRef = module.global.get(varName, wasmType);
        } else if (accessInfo instanceof LocalAccess) {
            const { index, wasmType } = accessInfo;
            loadRef = module.local.get(index, wasmType);
        } else if (accessInfo instanceof FunctionAccess) {
            const { funcScope } = accessInfo;
            loadRef = this.WASMFuncExpr(new FunctionExpression(funcScope));
        } else if (accessInfo instanceof StructAccess) {
            const { ref, fieldIndex, wasmType } = accessInfo;

            loadRef = binaryenCAPI._BinaryenStructGet(
                module.ptr,
                fieldIndex,
                ref,
                wasmType,
                false,
            );
        } else if (accessInfo instanceof InterfaceAccess) {
            const {
                infcTypeId,
                objTypeId,
                objRef,
                objType,
                fieldIndex,
                dynFieldIndex,
                tsType, // field Type
            } = accessInfo;
            const castedObjRef = binaryenCAPI._BinaryenRefCast(
                module.ptr,
                objRef,
                objType,
            );
            const wasmFieldType = this.wasmType.getWASMType(tsType);
            const ifTrue = binaryenCAPI._BinaryenStructGet(
                module.ptr,
                fieldIndex,
                castedObjRef,
                wasmFieldType,
                false,
            );
            const ifFalse = this.dynGetInfcField(objRef, dynFieldIndex, tsType);

            loadRef = createCondBlock(
                module,
                infcTypeId,
                objTypeId,
                ifTrue,
                ifFalse,
            );
        } else if (accessInfo instanceof ArrayAccess) {
            const { ref, index, wasmType } = accessInfo;

            loadRef = binaryenCAPI._BinaryenArrayGet(
                module.ptr,
                ref,
                index,
                wasmType,
                false,
            );
        } else if (accessInfo instanceof DynObjectAccess) {
            const { ref, fieldName } = accessInfo;
            if (fieldName === '__proto__') {
                loadRef = module.call(
                    dyntype.dyntype_get_prototype,
                    [
                        module.global.get(
                            dyntype.dyntype_context,
                            dyntype.dyn_ctx_t,
                        ),
                        ref,
                    ],
                    dyntype.dyn_value_t,
                );
            } else {
                const propNameStr = module.i32.const(
                    this.wasmCompiler.generateRawString(fieldName),
                );

                loadRef = module.call(
                    dyntype.dyntype_get_property,
                    [
                        module.global.get(
                            dyntype.dyntype_context,
                            dyntype.dyn_ctx_t,
                        ),
                        ref,
                        propNameStr,
                    ],
                    dyntype.dyn_value_t,
                );
            }
        } else if (accessInfo instanceof GetterAccess) {
            const { methodType, methodIndex, classType, thisObj } = accessInfo;
            if (!thisObj) {
                throw new Error(
                    `object is null when accessing getter method of class, class name is '${classType.className}'`,
                );
            }
            loadRef = this._generateClassMethodCallRef(
                thisObj,
                classType,
                methodType,
                methodIndex,
                [
                    binaryenCAPI._BinaryenRefNull(
                        module.ptr,
                        emptyStructType.typeRef,
                    ),
                    thisObj!,
                ],
            );
        } else if (accessInfo instanceof MethodAccess) {
            const { methodType, methodIndex, classType, thisObj } = accessInfo;
            if (accessInfo.isBuiltInMethod) {
                /** builtin instance field invoke */
                const mangledMethodName = accessInfo.mangledMethodName;
                switch (mangledMethodName) {
                    case BuiltinNames.builtinModuleName.concat(
                        BuiltinNames.moduleDelimiter,
                        BuiltinNames.stringLengthFuncName,
                    ):
                        loadRef = this._getStringRefLen(thisObj!);
                        break;
                    case BuiltinNames.builtinModuleName.concat(
                        BuiltinNames.moduleDelimiter,
                        BuiltinNames.arrayLengthFuncName,
                    ):
                        loadRef = this._getArrayRefLen(thisObj!);
                        break;
                }
            } else {
                const vtable = this.wasmType.getWASMClassVtable(classType);
                const wasmMethodType = this.wasmType.getWASMType(methodType);
                const targetFunction = binaryenCAPI._BinaryenStructGet(
                    this.module.ptr,
                    methodIndex,
                    vtable,
                    wasmMethodType,
                    false,
                );
                loadRef = targetFunction;
            }
        } else if (accessInfo instanceof InfcGetterAccess) {
            const {
                infcTypeId,
                objTypeId,
                objRef,
                objType,
                methodIndex,
                dynMethodIndex,
                infcType,
                methodType,
            } = accessInfo;
            const refnull = binaryenCAPI._BinaryenRefNull(
                this.module.ptr,
                emptyStructType.typeRef,
            );
            const castedObjRef = binaryenCAPI._BinaryenRefCast(
                this.module.ptr,
                objRef,
                objType,
            );
            const callWasmArgs = [refnull, castedObjRef];
            const ifTrue = this._generateClassMethodCallRef(
                castedObjRef,
                infcType,
                methodType,
                methodIndex,
                callWasmArgs,
            );
            const dynTargetField = this.dynGetInfcField(
                objRef,
                dynMethodIndex,
                methodType,
            );
            callWasmArgs[1] = binaryenCAPI._BinaryenRefCast(
                this.module.ptr,
                objRef,
                emptyStructType.typeRef,
            );
            const ifFalse = binaryenCAPI._BinaryenCallRef(
                this.module.ptr,
                dynTargetField,
                arrayToPtr(callWasmArgs).ptr,
                callWasmArgs.length,
                this.wasmType.getWASMType(methodType),
                false,
            );
            loadRef = createCondBlock(
                this.module,
                infcTypeId,
                objTypeId,
                ifTrue,
                ifFalse,
            );
        } else if (accessInfo instanceof DynArrayAccess) {
            throw Error(`dynamic array not implemented`);
        } else {
            return accessInfo;
        }

        if (loadRef === 0) {
            throw Error(`Failed to load value from AccessInfo`);
        }

        return loadRef;
    }

    private _createAccessInfo(
        identifer: string,
        scope: Scope,
        nested = true,
    ): AccessBase {
        /* Step1: Find item according to identifier */
        const identifierInfo = scope.findIdentifier(identifer, nested);
        if (identifierInfo instanceof Variable) {
            const variable = identifierInfo;
            let varType = this.wasmType.getWASMType(variable.varType);
            if (variable.varType instanceof TSFunction) {
                varType = this.wasmType.getWASMFuncStructType(variable.varType);
            }

            if (!variable.isLocalVar()) {
                return new GlobalAccess(
                    variable.mangledName,
                    varType,
                    variable.varType,
                );
            } else if (variable.varIsClosure) {
                const closureScope = variable.scope!;
                const closureIndex = variable.getClosureIndex();
                const currentScope = this.currentFuncCtx.getCurrentScope();
                const tsType = variable.varType;
                let scope: Scope | null = currentScope;

                /* Get current scope's context variable */
                let contextType = WASMGen.contextOfScope.get(scope)!.typeRef;
                let contextRef = this.module.local.get(
                    (scope as ClosureEnvironment).contextVariable!.varIndex,
                    contextType,
                );

                while (scope?.getNearestFunctionScope()) {
                    if (scope.kind === ScopeKind.ClassScope) {
                        scope = scope!.parent;
                        continue;
                    }
                    contextType = WASMGen.contextOfScope.get(scope!)!.typeRef;

                    if (scope !== closureScope) {
                        if ((scope as ClosureEnvironment).hasFreeVar) {
                            contextRef = binaryenCAPI._BinaryenStructGet(
                                this.module.ptr,
                                0,
                                contextRef,
                                contextType,
                                false,
                            );
                        }
                    } else {
                        /* Variable is defined in this scope, covert to StructAccess */
                        return new StructAccess(
                            contextRef,
                            closureIndex,
                            contextType,
                            tsType,
                        );
                    }
                    scope = scope!.parent;
                }

                throw Error(`Can't find closure scope`);
            } else {
                /* Local variable */
                return new LocalAccess(
                    variable.varIndex,
                    varType,
                    variable.varType,
                );
            }
        } else if (identifierInfo instanceof FunctionScope) {
            return new FunctionAccess(identifierInfo);
        } else if (
            identifierInfo instanceof NamespaceScope ||
            identifierInfo instanceof GlobalScope
        ) {
            return new ScopeAccess(identifierInfo);
        } else if (identifierInfo instanceof Type) {
            const tsType = identifierInfo;
            return new TypeAccess(tsType);
        } else {
            throw new Error(`Can't find identifier <"${identifer}">`);
        }
    }

    /* If byRef === true, return AccessInfo for left-value, but right-value is still returned by value */
    private WASMIdenfierExpr(
        expr: IdentifierExpression,
        byRef = false,
    ): binaryen.ExpressionRef | AccessBase {
        // find the target scope
        const currentScope = this.currentFuncCtx.getCurrentScope();
        const accessInfo = this._createAccessInfo(
            expr.identifierName,
            currentScope,
            true,
        );
        if (!byRef) {
            return this._loadFromAccessInfo(accessInfo);
        }

        return accessInfo;
    }

    private WASMBinaryExpr(expr: BinaryExpression): binaryen.ExpressionRef {
        const leftExpr = expr.leftOperand;
        const rightExpr = expr.rightOperand;
        const operatorKind = expr.operatorKind;
        const leftExprType = leftExpr.exprType;
        const rightExprType = rightExpr.exprType;
        let rightExprRef = this.WASMExprGen(rightExpr).binaryenRef;
        switch (operatorKind) {
            case ts.SyntaxKind.EqualsToken: {
                /*
                 a = b++  ==>
                 block {
                    a = b;
                    b = b + 1;
                 }
                 a = ++b  ==>
                 block {
                    b = b + 1;
                    a = b;
                 }
                */
                const assignWASMExpr = this.assignBinaryExpr(
                    leftExpr,
                    rightExpr,
                    leftExprType,
                    rightExprType,
                    rightExprRef,
                );
                if (
                    rightExpr.expressionKind ===
                        ts.SyntaxKind.PostfixUnaryExpression ||
                    rightExpr.expressionKind ===
                        ts.SyntaxKind.PrefixUnaryExpression
                ) {
                    const unaryExpr = <UnaryExpression>rightExpr;
                    /* iff  ExclamationToken, no need this step*/
                    if (
                        unaryExpr.operatorKind !==
                            ts.SyntaxKind.PlusPlusToken &&
                        unaryExpr.operatorKind !== ts.SyntaxKind.MinusMinusToken
                    ) {
                        return assignWASMExpr;
                    }
                    const operandExpr = unaryExpr.operand;
                    const operandExprType = unaryExpr.operand.exprType;
                    const rightUnaryAssignWASMExpr = this.assignBinaryExpr(
                        leftExpr,
                        operandExpr,
                        leftExprType,
                        operandExprType,
                    );
                    /* a = ++b  ==>
                        block {
                            b = b + 1;
                            a = b;
                        }
                    */
                    if (
                        unaryExpr.expressionKind ===
                        ts.SyntaxKind.PrefixUnaryExpression
                    ) {
                        return this.module.block(null, [
                            rightExprRef,
                            rightUnaryAssignWASMExpr,
                        ]);
                    } else {
                        return this.module.block(null, [
                            rightUnaryAssignWASMExpr,
                            rightExprRef,
                        ]);
                    }
                }
                return assignWASMExpr;
            }
            case ts.SyntaxKind.PlusEqualsToken: {
                const equalTokenRightExpr = new BinaryExpression(
                    ts.SyntaxKind.PlusToken,
                    leftExpr,
                    rightExpr,
                );
                equalTokenRightExpr.setExprType(leftExprType);
                return this.assignBinaryExpr(
                    leftExpr,
                    equalTokenRightExpr,
                    leftExprType,
                    rightExprType,
                );
            }
            case ts.SyntaxKind.MinusEqualsToken: {
                const equalTokenRightExpr = new BinaryExpression(
                    ts.SyntaxKind.MinusToken,
                    leftExpr,
                    rightExpr,
                );
                equalTokenRightExpr.setExprType(leftExprType);
                return this.assignBinaryExpr(
                    leftExpr,
                    equalTokenRightExpr,
                    leftExprType,
                    rightExprType,
                );
            }
            case ts.SyntaxKind.AsteriskEqualsToken: {
                const equalTokenRightExpr = new BinaryExpression(
                    ts.SyntaxKind.AsteriskToken,
                    leftExpr,
                    rightExpr,
                );
                equalTokenRightExpr.setExprType(leftExprType);
                return this.assignBinaryExpr(
                    leftExpr,
                    equalTokenRightExpr,
                    leftExprType,
                    rightExprType,
                );
            }
            case ts.SyntaxKind.SlashEqualsToken: {
                const equalTokenRightExpr = new BinaryExpression(
                    ts.SyntaxKind.SlashToken,
                    leftExpr,
                    rightExpr,
                );
                equalTokenRightExpr.setExprType(leftExprType);
                return this.assignBinaryExpr(
                    leftExpr,
                    equalTokenRightExpr,
                    leftExprType,
                    rightExprType,
                );
            }
            default: {
                let leftExprRef = this.WASMExprGen(leftExpr).binaryenRef;

                if (
                    leftExpr.expressionKind ===
                        ts.SyntaxKind.PostfixUnaryExpression ||
                    leftExpr.expressionKind ===
                        ts.SyntaxKind.PrefixUnaryExpression
                ) {
                    const unaryExpr = <UnaryExpression>leftExpr;
                    if (
                        unaryExpr.operatorKind ===
                            ts.SyntaxKind.PlusPlusToken ||
                        unaryExpr.operatorKind === ts.SyntaxKind.MinusMinusToken
                    ) {
                        leftExprRef = <binaryen.ExpressionRef>(
                            this._generateUnaryExprBlock(unaryExpr, leftExprRef)
                        );
                    }
                }
                if (
                    rightExpr.expressionKind ===
                        ts.SyntaxKind.PostfixUnaryExpression ||
                    rightExpr.expressionKind ===
                        ts.SyntaxKind.PrefixUnaryExpression
                ) {
                    const unaryExpr = <UnaryExpression>rightExpr;
                    if (
                        unaryExpr.operatorKind ===
                            ts.SyntaxKind.PlusPlusToken ||
                        unaryExpr.operatorKind === ts.SyntaxKind.MinusMinusToken
                    ) {
                        rightExprRef = <binaryen.ExpressionRef>(
                            this._generateUnaryExprBlock(
                                unaryExpr,
                                rightExprRef,
                            )
                        );
                    }
                }
                return this.operateBinaryExpr(
                    leftExprRef,
                    rightExprRef,
                    operatorKind,
                    leftExprType,
                    rightExprType,
                );
            }
        }
    }

    private operateBinaryExpr(
        leftExprRef: binaryen.ExpressionRef,
        rightExprRef: binaryen.ExpressionRef,
        operatorKind: ts.SyntaxKind,
        leftExprType: Type,
        rightExprType: Type,
    ): binaryen.ExpressionRef {
        if (
            leftExprType.kind === TypeKind.NUMBER &&
            rightExprType.kind === TypeKind.NUMBER
        ) {
            return this.operateF64F64(leftExprRef, rightExprRef, operatorKind);
        }
        if (
            leftExprType.kind === TypeKind.NUMBER &&
            rightExprType.kind === TypeKind.BOOLEAN
        ) {
            return this.operateF64I32(leftExprRef, rightExprRef, operatorKind);
        }
        if (
            leftExprType.kind === TypeKind.BOOLEAN &&
            rightExprType.kind === TypeKind.NUMBER
        ) {
            return this.operateI32F64(leftExprRef, rightExprRef, operatorKind);
        }
        if (
            leftExprType.kind === TypeKind.BOOLEAN &&
            rightExprType.kind === TypeKind.BOOLEAN
        ) {
            return this.operateI32I32(leftExprRef, rightExprRef, operatorKind);
        }
        if (
            leftExprType.kind === TypeKind.ANY &&
            rightExprType.kind === TypeKind.ANY
        ) {
            return this.operateAnyAny(leftExprRef, rightExprRef, operatorKind);
        }
        if (
            leftExprType.kind === TypeKind.ANY &&
            rightExprType.kind === TypeKind.NUMBER
        ) {
            return this.operateAnyNumber(
                leftExprRef,
                rightExprRef,
                operatorKind,
            );
        }
        if (
            leftExprType.kind === TypeKind.NUMBER &&
            rightExprType.kind === TypeKind.ANY
        ) {
            return this.operateAnyNumber(
                rightExprRef,
                leftExprRef,
                operatorKind,
            );
        }
        throw new Error(
            'unexpected left expr type ' +
                leftExprType.kind +
                'unexpected right expr type ' +
                rightExprType.kind,
        );
    }

    private assignBinaryExpr(
        leftExpr: Expression,
        rightExpr: Expression,
        leftExprType: Type,
        rightExprType: Type,
        rightExprRef?: binaryen.ExpressionRef,
    ): binaryen.ExpressionRef {
        const module = this.module;
        const matchKind = this.matchType(leftExprType, rightExprType);
        if (matchKind === MatchKind.MisMatch) {
            throw new Error('Type mismatch in ExpressionStatement');
        }

        let assignValue: binaryen.ExpressionRef;
        if (matchKind === MatchKind.ToAnyMatch) {
            assignValue =
                this.dynValueGen.WASMDynExprGen(rightExpr).binaryenRef;
        } else {
            if (rightExprRef) {
                assignValue = rightExprRef;
            } else {
                assignValue = this.WASMExprGen(rightExpr).binaryenRef;
            }
        }
        if (matchKind === MatchKind.ClassInfcMatch) {
            assignValue = this.maybeTypeBoxingAndUnboxing(
                <TSClass>rightExprType,
                <TSClass>leftExprType,
                assignValue,
            );
        }
        const accessInfo = this.WASMExprGenInternal(leftExpr, true);
        if (accessInfo instanceof GlobalAccess) {
            const { varName } = accessInfo;
            return module.global.set(varName, assignValue);
        } else if (accessInfo instanceof LocalAccess) {
            const { index } = accessInfo;
            return module.local.set(index, assignValue);
        } else if (accessInfo instanceof StructAccess) {
            const { ref, fieldIndex } = accessInfo;

            return binaryenCAPI._BinaryenStructSet(
                module.ptr,
                fieldIndex,
                ref,
                assignValue,
            );
        } else if (accessInfo instanceof InterfaceAccess) {
            const {
                infcTypeId,
                objTypeId,
                objRef,
                objType,
                fieldIndex,
                dynFieldIndex,
                tsType, // field Type
            } = accessInfo;
            const castedObjRef = binaryenCAPI._BinaryenRefCast(
                module.ptr,
                objRef,
                objType,
            );
            const ifTrue = binaryenCAPI._BinaryenStructSet(
                module.ptr,
                fieldIndex,
                castedObjRef,
                assignValue,
            );
            const ifFalse = this.dynSetInfcField(
                objRef,
                dynFieldIndex,
                assignValue,
                tsType,
            );

            return module.if(
                module.i32.eq(infcTypeId, objTypeId),
                ifTrue,
                ifFalse,
            );
        } else if (accessInfo instanceof ArrayAccess) {
            const { ref, index } = accessInfo;
            /** TODO: arrays get from `Array.of` may grow dynamiclly*/
            return binaryenCAPI._BinaryenArraySet(
                module.ptr,
                ref,
                index,
                assignValue,
            );
        } else if (accessInfo instanceof DynObjectAccess) {
            const { ref, fieldName } = accessInfo;
            if (fieldName === '__proto__') {
                return module.drop(
                    module.call(
                        dyntype.dyntype_set_prototype,
                        [
                            module.global.get(
                                dyntype.dyntype_context,
                                dyntype.dyn_ctx_t,
                            ),
                            ref,
                            assignValue,
                        ],
                        dyntype.int,
                    ),
                );
            }
            const propNameStr = module.i32.const(
                this.wasmCompiler.generateRawString(fieldName),
            );
            const setPropertyExpression = module.drop(
                module.call(
                    dyntype.dyntype_set_property,
                    [
                        module.global.get(
                            dyntype.dyntype_context,
                            dyntype.dyn_ctx_t,
                        ),
                        ref,
                        propNameStr,
                        this.dynValueGen.WASMDynExprGen(rightExpr)!.binaryenRef,
                    ],
                    dyntype.int,
                ),
            );
            return setPropertyExpression;
        } else if (accessInfo instanceof DynArrayAccess) {
            throw Error(`Dynamic array not implemented`);
        } else {
            /* TODO: print the related source code */
            throw new Error(`Invalid assign target`);
        }
    }

    private matchType(leftExprType: Type, rightExprType: Type): number {
        /** iff tsc checking is OK, the leftside is any or reference type, both are OK */
        if (rightExprType.kind === TypeKind.NULL) {
            return MatchKind.ExactMatch;
        }
        if (leftExprType.kind === rightExprType.kind) {
            if (
                leftExprType.kind === TypeKind.NUMBER ||
                leftExprType.kind === TypeKind.STRING ||
                leftExprType.kind === TypeKind.BOOLEAN ||
                leftExprType.kind === TypeKind.ANY ||
                leftExprType.kind === TypeKind.INTERFACE
            ) {
                return MatchKind.ExactMatch;
            } else if (leftExprType.kind === TypeKind.ARRAY) {
                const leftArrayType = <TSArray>leftExprType;
                const rightArrayType = <TSArray>rightExprType;
                if (leftArrayType.elementType === rightArrayType.elementType) {
                    return MatchKind.ExactMatch;
                }
                if (leftArrayType.elementType.kind === TypeKind.ANY) {
                    return MatchKind.ToArrayAnyMatch;
                }
                if (rightArrayType.elementType.kind === TypeKind.ANY) {
                    return MatchKind.FromArrayAnyMatch;
                }

                return this.matchType(
                    leftArrayType.elementType,
                    rightArrayType.elementType,
                );
            } else if (leftExprType.kind === TypeKind.CLASS) {
                const leftClassType = <TSClass>leftExprType;
                const rightClassType = <TSClass>rightExprType;
                const leftClassName = leftClassType.mangledName;
                const rightClassName = rightClassType.mangledName;
                if (leftClassName === rightClassName) {
                    return MatchKind.ClassMatch;
                }
                /* iff explicit subtyping, such as class B extends A ==> it allows: a(A) = b(B)  */
                let rightClassBaseType = rightClassType.getBase();
                while (rightClassBaseType !== null) {
                    if (rightClassBaseType.mangledName === leftClassName) {
                        return MatchKind.ClassInheritMatch;
                    }
                    rightClassBaseType = rightClassBaseType.getBase();
                }
                return MatchKind.MisMatch;
            } else if (leftExprType.kind === TypeKind.FUNCTION) {
                const leftFuncType = <TSFunction>leftExprType;
                const rightFuncType = <TSFunction>rightExprType;
                if (
                    this.matchType(
                        leftFuncType.returnType,
                        rightFuncType.returnType,
                    ) == MatchKind.MisMatch
                ) {
                    return MatchKind.MisMatch;
                }

                const leftParams = leftFuncType.getParamTypes();
                const rightParams = rightFuncType.getParamTypes();
                if (leftParams.length !== rightParams.length) {
                    return MatchKind.MisMatch;
                }

                for (let i = 0; i < leftParams.length; i++) {
                    if (
                        this.matchType(leftParams[i], rightParams[i]) ==
                        MatchKind.MisMatch
                    ) {
                        return MatchKind.MisMatch;
                    }
                }

                // TODO: check rest parameters
                return MatchKind.ExactMatch;
            }
        }
        if (
            (leftExprType.kind === TypeKind.CLASS &&
                rightExprType.kind === TypeKind.INTERFACE) ||
            (leftExprType.kind === TypeKind.INTERFACE &&
                rightExprType.kind === TypeKind.CLASS)
        ) {
            return MatchKind.ClassInfcMatch;
        }
        if (leftExprType.kind === TypeKind.ANY) {
            return MatchKind.ToAnyMatch;
        }
        if (rightExprType.kind === TypeKind.ANY) {
            return MatchKind.FromAnyMatch;
        }
        return MatchKind.MisMatch;
    }

    private WASMUnaryExpr(expr: UnaryExpression): binaryen.ExpressionRef {
        const operator: ts.SyntaxKind = expr.operatorKind;
        const operand: Expression = expr.operand;
        switch (operator) {
            case ts.SyntaxKind.PlusPlusToken: {
                /* i++ ===> i += 1 */
                const numberExpr = new NumberLiteralExpression(1);
                numberExpr.setExprType(expr.exprType);
                const binaryExpr = new BinaryExpression(
                    ts.SyntaxKind.PlusEqualsToken,
                    operand,
                    numberExpr,
                );
                binaryExpr.setExprType(expr.exprType);
                return this.WASMBinaryExpr(binaryExpr);
            }
            case ts.SyntaxKind.MinusMinusToken: {
                /* i-- ===> i -= 1 */
                const numberExpr = new NumberLiteralExpression(1);
                numberExpr.setExprType(expr.exprType);
                const binaryExpr = new BinaryExpression(
                    ts.SyntaxKind.MinusEqualsToken,
                    operand,
                    numberExpr,
                );
                binaryExpr.setExprType(expr.exprType);
                return this.WASMBinaryExpr(binaryExpr);
            }
            case ts.SyntaxKind.ExclamationToken: {
                let WASMOperandExpr = this.WASMExprGen(operand).binaryenRef;
                WASMOperandExpr = this.generateCondition(WASMOperandExpr);
                return this.module.i32.eqz(WASMOperandExpr);
            }
            case ts.SyntaxKind.MinusToken: {
                if (operand.expressionKind === ts.SyntaxKind.NumericLiteral) {
                    const value: number = (<NumberLiteralExpression>operand)
                        .expressionValue;
                    return this.module.f64.const(-value);
                } else {
                    const WASMOperandExpr =
                        this.WASMExprGen(operand).binaryenRef;
                    return this.module.f64.sub(
                        this.module.f64.const(0),
                        WASMOperandExpr,
                    );
                }
            }
            case ts.SyntaxKind.PlusToken: {
                return this.WASMExprGen(operand).binaryenRef;
            }
        }
        return this.module.unreachable();
    }

    private WASMConditionalExpr(
        expr: ConditionalExpression,
    ): binaryen.ExpressionRef {
        let condWASMExpr = this.WASMExprGen(expr.condtion).binaryenRef;
        // convert to condition
        condWASMExpr = this.generateCondition(condWASMExpr);
        const trueWASMExpr = this.WASMExprGen(expr.whenTrue).binaryenRef;
        const falseWASMExpr = this.WASMExprGen(expr.whenFalse).binaryenRef;
        // TODO: union type
        assert(
            binaryen.getExpressionType(trueWASMExpr) ===
                binaryen.getExpressionType(falseWASMExpr),
            'trueWASMExprType and falseWASMExprType are not equal in conditional expression ',
        );
        return this.module.select(condWASMExpr, trueWASMExpr, falseWASMExpr);
    }

    private WASMCallExpr(expr: CallExpression): binaryen.ExpressionRef {
        const callExpr = expr.callExpr;

        if (!(callExpr.exprType instanceof TSFunction)) {
            Logger.error(`call non-function`);
        }
        let callWasmArgs = this.parseArguments(
            callExpr.exprType as TSFunction,
            expr.callArgs,
        );
        /* In call expression, the callee may be a function scope rather than a variable,
            we use WASMExprGenInternal here which may return a FunctionAccess object */
        const accessInfo = this.WASMExprGenInternal(callExpr, true);
        const context = binaryenCAPI._BinaryenRefNull(
            this.module.ptr,
            emptyStructType.typeRef,
        );
        if (accessInfo instanceof AccessBase) {
            /** TODO: get default parameter information, generate new call args
             *  default parameter information should be recorded in type information
             */
            if (accessInfo instanceof MethodAccess) {
                const { methodType, methodIndex, classType, thisObj } =
                    accessInfo;
                let finalCallWasmArgs = [];
                if (!methodType.isDeclare) {
                    finalCallWasmArgs.push(context);
                }
                if (thisObj) {
                    finalCallWasmArgs.push(thisObj);
                }
                finalCallWasmArgs = finalCallWasmArgs.concat(callWasmArgs);
                if (accessInfo.isBuiltInMethod) {
                    return this.module.call(
                        accessInfo.mangledMethodName,
                        finalCallWasmArgs,
                        this.wasmType.getWASMFuncReturnType(methodType),
                    );
                } else {
                    return this._generateClassMethodCallRef(
                        thisObj,
                        classType,
                        methodType,
                        methodIndex,
                        finalCallWasmArgs,
                    );
                }
            } else if (accessInfo instanceof InfcMethodAccess) {
                const {
                    infcTypeId,
                    objTypeId,
                    objRef,
                    objType,
                    methodIndex,
                    dynMethodIndex,
                    infcType,
                    methodType,
                } = accessInfo;
                const refnull = binaryenCAPI._BinaryenRefNull(
                    this.module.ptr,
                    emptyStructType.typeRef,
                );
                const castedObjRef = binaryenCAPI._BinaryenRefCast(
                    this.module.ptr,
                    objRef,
                    objType,
                );
                callWasmArgs = [refnull, castedObjRef, ...callWasmArgs];
                const ifTrue = this._generateClassMethodCallRef(
                    castedObjRef,
                    infcType,
                    methodType,
                    methodIndex,
                    callWasmArgs,
                );
                const dynTargetField = this.dynGetInfcField(
                    objRef,
                    dynMethodIndex,
                    methodType,
                );
                callWasmArgs[1] = binaryenCAPI._BinaryenRefCast(
                    this.module.ptr,
                    objRef,
                    emptyStructType.typeRef,
                );
                const ifFalse = binaryenCAPI._BinaryenCallRef(
                    this.module.ptr,
                    dynTargetField,
                    arrayToPtr(callWasmArgs).ptr,
                    callWasmArgs.length,
                    this.wasmType.getWASMType(methodType),
                    false,
                );
                return createCondBlock(
                    this.module,
                    infcTypeId,
                    objTypeId,
                    ifTrue,
                    ifFalse,
                );
            } else {
                return this._generateFuncCall(
                    accessInfo,
                    context,
                    callWasmArgs,
                    expr,
                );
            }
        } else {
            return this._generateFuncCall(
                accessInfo,
                context,
                callWasmArgs,
                expr,
            );
        }
    }

    private WASMArrayLiteralExpr(
        expr: ArrayLiteralExpression,
    ): binaryen.ExpressionRef {
        const arrType = expr.exprType;
        const elements = expr.arrayValues;
        let res: binaryen.ExpressionRef;
        if (arrType.kind === TypeKind.ANY) {
            res = this.dynValueGen.WASMDynExprGen(expr).binaryenRef;
        } else {
            res = this.initArray(arrType as TSArray, elements);
        }
        return res;
    }

    private WASMObjectLiteralExpr(
        expr: ObjectLiteralExpression,
    ): binaryen.ExpressionRef {
        const module = this.module;
        const objType = <TSClass>expr.exprType;
        // store members and methods seperately
        const propRefList: binaryen.ExpressionRef[] = [binaryen.none];
        const vtable: binaryen.ExpressionRef[] = [];

        const fields = expr.objectFields;
        const values = expr.objectValues;
        const propertyLen = fields.length;
        for (let i = 0; i < propertyLen; i++) {
            const propExpr = values[i];
            const propExprType = propExpr.exprType;
            /* TODO: not parse member function yet */
            if (propExprType.kind === TypeKind.FUNCTION) {
                const methodStruct = this.WASMExprGen(propExpr).binaryenRef;
                const temp = binaryenCAPI._BinaryenStructGet(
                    module.ptr,
                    1,
                    methodStruct,
                    binaryen.getExpressionType(methodStruct),
                    false,
                );
                vtable.push(temp);
            } else {
                let propExprRef: binaryen.ExpressionRef;
                if (propExprType.kind === TypeKind.ANY) {
                    propExprRef =
                        this.dynValueGen.WASMDynExprGen(propExpr).binaryenRef;
                } else {
                    propExprRef = this.WASMExprGen(propExpr).binaryenRef;
                }
                propRefList.push(propExprRef);
            }
        }
        const vtableHeapType =
            this.wasmType.getWASMClassVtableHeapType(objType);
        const objHeapType = this.wasmType.getWASMHeapType(objType);
        propRefList[0] = binaryenCAPI._BinaryenStructNew(
            module.ptr,
            arrayToPtr(vtable).ptr,
            vtable.length,
            vtableHeapType,
        );
        const objectLiteralValue = binaryenCAPI._BinaryenStructNew(
            module.ptr,
            arrayToPtr(propRefList).ptr,
            propRefList.length,
            objHeapType,
        );
        return objectLiteralValue;
    }

    private WASMSuperExpr(expr: SuperCallExpression): binaryen.ExpressionRef {
        // must in a constructor
        const module = this.module;
        const scope = <FunctionScope>this.currentFuncCtx.getCurrentScope();
        const classScope = <ClassScope>scope.getNearestFunctionScope()!.parent;
        const classType = classScope.classType;
        const baseClassType = <TSClass>classType.getBase();
        const wasmBaseTypeRef = this.wasmType.getWASMType(baseClassType);
        // 0: @context 1: @this
        const ref = module.local.get(1, emptyStructType.typeRef);
        const cast = binaryenCAPI._BinaryenRefCast(
            module.ptr,
            ref,
            wasmBaseTypeRef,
        );
        const wasmArgs = new Array<binaryen.ExpressionRef>();
        wasmArgs.push(
            binaryenCAPI._BinaryenRefNull(module.ptr, emptyStructType.typeRef),
        );
        wasmArgs.push(cast);
        for (const arg of expr.callArgs) {
            wasmArgs.push(this.WASMExprGen(arg).binaryenRef);
        }
        return module.drop(
            module.call(
                baseClassType.mangledName + '|constructor',
                wasmArgs,
                binaryen.none,
            ),
        );
    }

    private WASMNewExpr(expr: NewExpression): binaryen.ExpressionRef {
        const type = expr.exprType;
        const module = this.module;
        if (type.kind === TypeKind.ARRAY) {
            const arrayHeapType = this.wasmType.getWASMHeapType(type);
            if (expr.lenExpr) {
                const arraySize = this.convertTypeToI32(
                    this.WASMExprGen(expr.lenExpr).binaryenRef,
                    binaryen.f64,
                );
                const arrayInit = this.getArrayInitFromArrayType(<TSArray>type);
                return binaryenCAPI._BinaryenArrayNew(
                    module.ptr,
                    arrayHeapType,
                    arraySize,
                    arrayInit,
                    /* Note: We should use binaryen.none here, but currently
                        the corresponding opcode is not supported by runtime */
                );
            } else if (!expr.newArgs) {
                const arraySize = this.convertTypeToI32(
                    module.f64.const(expr.arrayLen),
                    binaryen.f64,
                );
                const arrayInit = this.getArrayInitFromArrayType(<TSArray>type);
                return binaryenCAPI._BinaryenArrayNew(
                    module.ptr,
                    arrayHeapType,
                    arraySize,
                    arrayInit,
                );
            } else {
                const arrayType = <TSArray>type;
                const arrayLen = expr.arrayLen;
                const array = [];
                for (let i = 0; i < expr.arrayLen; i++) {
                    const elemExpr = expr.newArgs[i];
                    let elemExprRef: binaryen.ExpressionRef;
                    if (arrayType.elementType.kind === TypeKind.ANY) {
                        elemExprRef =
                            this.dynValueGen.WASMDynExprGen(
                                elemExpr,
                            ).binaryenRef;
                    } else {
                        elemExprRef = this.WASMExprGen(elemExpr).binaryenRef;
                    }

                    array.push(elemExprRef);
                }
                const arrayValue = binaryenCAPI._BinaryenArrayInit(
                    module.ptr,
                    arrayHeapType,
                    arrayToPtr(array).ptr,
                    arrayLen,
                );
                return arrayValue;
            }
        }
        if (type.kind === TypeKind.CLASS) {
            const classType = <TSClass>type;
            const classMangledName = classType.mangledName;
            const initStructFields = new Array<binaryen.ExpressionRef>();
            initStructFields.push(this.wasmType.getWASMClassVtable(type));
            const classFields = classType.fields;
            for (const field of classFields) {
                initStructFields.push(this.defaultValue(field.type.kind));
            }
            const newStruct = binaryenCAPI._BinaryenStructNew(
                module.ptr,
                arrayToPtr(initStructFields).ptr,
                initStructFields.length,
                this.wasmType.getWASMHeapType(type),
            );

            const args = new Array<binaryen.ExpressionRef>();
            // TODO: here just set @context to null
            args.push(
                binaryenCAPI._BinaryenRefNull(
                    this.module.ptr,
                    emptyStructType.typeRef,
                ),
            );
            args.push(newStruct);
            if (expr.newArgs) {
                for (const arg of expr.newArgs) {
                    args.push(this.WASMExprGen(arg).binaryenRef);
                }
            }
            return this.module.call(
                classMangledName + '|constructor',
                args,
                this.wasmType.getWASMType(classType),
            );
        }
        return binaryen.none;
    }

    private WASMPropertyAccessExpr(
        expr: PropertyAccessExpression,
        byRef = false,
    ): binaryen.ExpressionRef | AccessBase {
        const module = this.module;
        const objPropAccessExpr = expr.propertyAccessExpr;
        const propExpr = expr.propertyExpr;
        const propIdenExpr = <IdentifierExpression>propExpr;
        const propName = propIdenExpr.identifierName;
        let curAccessInfo: AccessBase | null = null;

        const accessInfo = this.WASMExprGenInternal(objPropAccessExpr);
        if (accessInfo instanceof AccessBase) {
            if (accessInfo instanceof ScopeAccess) {
                curAccessInfo = this._createAccessInfo(
                    propName,
                    accessInfo.scope,
                    false,
                );
            } else if (accessInfo instanceof TypeAccess) {
                const type = accessInfo.type;
                if (type instanceof TSClass) {
                    const propIndex = type.getStaticFieldIndex(propName);
                    if (propIndex !== -1) {
                        // static field
                        const wasmStaticFieldsType =
                            this.wasmType.getWASMClassStaticFieldsType(type);
                        const ref = module.global.get(
                            `${type.mangledName}_static_fields`,
                            wasmStaticFieldsType,
                        );
                        const propType =
                            type.getStaticMemberField(propName)!.type;
                        curAccessInfo = new StructAccess(
                            ref,
                            propIndex,
                            this.wasmType.getWASMType(propType),
                            propType,
                        );
                    } else {
                        const methodInfo = type.getMethod(
                            propName,
                            FunctionKind.STATIC,
                        );
                        if (methodInfo.index === -1) {
                            throw new Error(
                                `static method of class '${type.className}' not found`,
                            );
                        }
                        curAccessInfo = new MethodAccess(
                            methodInfo.method!.type,
                            methodInfo.index,
                            type,
                            null,
                        );
                    }
                }
            } else if (accessInfo instanceof Type) {
                throw Error("Access type's builtin method unimplement");
            }
        } else {
            const wasmValue = accessInfo;
            const ref = wasmValue.binaryenRef;
            const tsType = wasmValue.tsType;
            const currentScope = this.currentFuncCtx.getCurrentScope();
            switch (tsType.typeKind) {
                case TypeKind.BOOLEAN:
                case TypeKind.NUMBER:
                case TypeKind.FUNCTION:
                case TypeKind.STRING:
                case TypeKind.ARRAY: {
                    const className = getClassNameByTypeKind(tsType.typeKind);
                    const classType = <TSClass>(
                        currentScope.findIdentifier(className)
                    );
                    curAccessInfo = new MethodAccess(
                        <TSFunction>propExpr.exprType,
                        classType.getMethod(
                            propName,
                            FunctionKind.METHOD,
                        ).index,
                        classType,
                        ref,
                        true,
                        propName,
                    );
                    break;
                }
                case TypeKind.CLASS: {
                    const classType = tsType as TSClass;
                    const propIndex = classType.getMemberFieldIndex(propName);
                    if (propIndex != -1) {
                        /* member field */
                        const propType =
                            classType.getMemberField(propName)!.type;
                        curAccessInfo = new StructAccess(
                            ref,
                            propIndex +
                                1 /* The first slot is reserved for vtable */,
                            this.wasmType.getWASMType(propType),
                            propType,
                        );
                    } else {
                        let classMethod = classType.getMethod(propName);
                        // iff xxx.setter()
                        if (classMethod.index === -1 && expr.accessSetter) {
                            classMethod = classType.getMethod(
                                propName,
                                FunctionKind.SETTER,
                            );
                        }
                        // call object literal method
                        if (classMethod.index === -1) {
                            classMethod = classType.getMethod(
                                propName,
                                FunctionKind.DEFAULT,
                            );
                        }
                        if (classMethod.index !== -1) {
                            curAccessInfo = new MethodAccess(
                                classMethod.method!.type,
                                classMethod.index,
                                classType,
                                ref,
                            );
                        } else {
                            classMethod = classType.getMethod(
                                propName,
                                FunctionKind.GETTER,
                            );
                            if (classMethod.index === -1) {
                                throw Error(
                                    `${propName} property does not exist on ${tsType}`,
                                );
                            }
                            curAccessInfo = new GetterAccess(
                                classMethod.method!.type,
                                classMethod.index,
                                classType,
                                ref,
                            );
                        }
                    }
                    break;
                }
                case TypeKind.INTERFACE: {
                    const infcType = tsType as TSInterface;
                    const ifcTypeId = this.module.i32.const(infcType.typeId);
                    const objTypeId = this.getInfcTypeId(ref);
                    const propIndex = infcType.getMemberFieldIndex(propName);
                    let dynFieldIndex = this.findItableIndex(ref, propName, 0);

                    const objRef = this.getInfcObj(ref); // anyref
                    const objType = this.wasmType.getWASMType(infcType, true);
                    if (propIndex != -1) {
                        const propType =
                            infcType.getMemberField(propName)!.type;
                        curAccessInfo = new InterfaceAccess(
                            ifcTypeId,
                            objTypeId,
                            objRef,
                            objType,
                            propIndex + 1,
                            dynFieldIndex,
                            propType,
                        );
                    } else {
                        let method = infcType.getMethod(propName);
                        dynFieldIndex = this.findItableIndex(ref, propName, 1);
                        if (method.index === -1 && expr.accessSetter) {
                            method = infcType.getMethod(
                                propName,
                                FunctionKind.SETTER,
                            );
                            dynFieldIndex = this.findItableIndex(
                                ref,
                                propName,
                                3,
                            );
                        }
                        if (method.index !== -1) {
                            curAccessInfo = new InfcMethodAccess(
                                ifcTypeId,
                                objTypeId,
                                objRef,
                                objType,
                                method.index,
                                dynFieldIndex,
                                infcType,
                                method.method!.type,
                            );
                        } else {
                            method = infcType.getMethod(
                                propName,
                                FunctionKind.GETTER,
                            );
                            if (method.index === -1) {
                                throw Error(
                                    `${propName} property does not exist on interface ${tsType}`,
                                );
                            }
                            dynFieldIndex = this.findItableIndex(
                                ref,
                                propName,
                                2,
                            );
                            curAccessInfo = new InfcGetterAccess(
                                ifcTypeId,
                                objTypeId,
                                objRef,
                                objType,
                                method.index,
                                dynFieldIndex,
                                infcType,
                                method.method!.type,
                            );
                        }
                    }
                    break;
                }
                case TypeKind.ANY:
                    curAccessInfo = new DynObjectAccess(ref, propName);
                    break;
                default:
                    throw Error(
                        `invalid property access, receiver type is: ${tsType.typeKind}`,
                    );
            }
        }

        if (!curAccessInfo) {
            throw Error(
                `unexpected error during processing propertyAccessExpression`,
            );
        }

        if (!byRef) {
            return this._loadFromAccessInfo(curAccessInfo);
        }

        return curAccessInfo;
    }

    private WASMElementAccessExpr(
        expr: ElementAccessExpression,
        byRef = false,
    ): binaryen.ExpressionRef | AccessBase {
        const module = this.module;
        const accessExpr = expr.accessExpr;
        const argExpr = expr.argExpr;
        const wasmValue = this.WASMExprGen(accessExpr);
        const arrayRef = wasmValue.binaryenRef;
        const arrayType = wasmValue.tsType;
        const index = this.convertTypeToI32(
            this.WASMExprGen(argExpr).binaryenRef,
            binaryen.f64,
        );

        if (arrayType instanceof TSArray) {
            const elementType = arrayType.elementType;
            const elemWasmType = this.wasmType.getWASMType(elementType);

            if (!byRef) {
                return binaryenCAPI._BinaryenArrayGet(
                    module.ptr,
                    arrayRef,
                    index,
                    elemWasmType,
                    false,
                );
            } else {
                return new ArrayAccess(
                    arrayRef,
                    index,
                    elemWasmType,
                    elementType,
                );
            }
        } else {
            /* Any-objects */
            if (!byRef) {
                throw Error(`Dynamic array not implemented`);
            } else {
                return new DynArrayAccess(arrayRef, index);
            }
        }
    }

    private WASMAsExpr(expr: AsExpression): binaryen.ExpressionRef {
        const originObjExpr = <IdentifierExpression>expr.expression;
        const originObjExprRef = this.WASMExprGen(originObjExpr).binaryenRef;
        const originType = originObjExpr.exprType;
        const targetType = expr.exprType;
        switch (originType.kind) {
            case TypeKind.ANY: {
                if (!this.wasmType.hasHeapType(targetType)) {
                    /** if target type is basic type, no need to cast */
                    return this.unboxAnyToBase(
                        originObjExprRef,
                        targetType.kind,
                    );
                } else {
                    /** if target type has heap type, need to call to_extref */
                    return this.unboxAnyToExtref(originObjExprRef, targetType);
                }
            }
            default: {
                throw Error(`Static type doesn't support type assertion`);
            }
        }
    }

    private WASMFuncExpr(expr: FunctionExpression): binaryen.ExpressionRef {
        const funcScope = expr.funcScope;
        const parentScope = funcScope.parent;
        let funcName = funcScope.mangledName;
        let funcType = funcScope.funcType;

        /** if function is declare,
         *  we create a wrapper function to keep the same calling convention */
        if (funcScope.isDeclare()) {
            const { wrapperName, wrapperType } =
                this.wasmCompiler.generateImportWrapper(funcScope);
            funcName = wrapperName;
            funcType = wrapperType;
        }

        const wasmFuncType = this.wasmType.getWASMType(funcType);

        const funcStructHeapType =
            this.wasmType.getWASMFuncStructHeapType(funcType);
        const funcStructType = this.wasmType.getWASMFuncStructType(funcType);
        let context = binaryenCAPI._BinaryenRefNull(
            this.module.ptr,
            emptyStructType.typeRef,
        );
        if (parentScope instanceof ClosureEnvironment) {
            const ce = parentScope;
            const index = ce.contextVariable!.varIndex;
            const type = ce.contextVariable!.varType;
            context = this.module.local.get(
                index,
                this.wasmType.getWASMType(type),
            );
        }
        const closureVar = new Variable(
            `@closure|${funcName}`,
            funcType,
            [],
            -1,
            true,
        );
        this.addVariableToCurrentScope(closureVar);

        const closureRef = binaryenCAPI._BinaryenStructNew(
            this.module.ptr,
            arrayToPtr([context, this.module.ref.func(funcName, wasmFuncType)])
                .ptr,
            2,
            funcStructHeapType,
        );

        this.currentFuncCtx.insert(
            this.module.local.set(closureVar.varIndex, closureRef),
        );
        return this.module.local.get(closureVar.varIndex, funcStructType);
    }

    private _generateInfcArgs(
        paramTypes: Type[],
        callArgs: Expression[],
        callWasmArgs: binaryen.ExpressionRef[],
    ) {
        for (let i = 0; i < callArgs.length; i++) {
            const paramType = paramTypes[i];
            const argType = callArgs[i].exprType;
            if (paramType instanceof TSClass && argType instanceof TSClass) {
                callWasmArgs[i] = this.maybeTypeBoxingAndUnboxing(
                    argType,
                    paramType,
                    callWasmArgs[i],
                );
            }
        }
        return callWasmArgs;
    }

    private _generateFuncCall(
        accessInfo: AccessBase | WasmValue,
        context: binaryen.ExpressionRef,
        callWasmArgs: binaryen.ExpressionRef[],
        expr: CallExpression,
    ) {
        const funcType = expr.callExpr.exprType as TSFunction;
        const paramTypes = funcType.getParamTypes();

        let funcRef: binaryen.ExpressionRef = -1;
        if (accessInfo instanceof AccessBase) {
            const wasmRef = this._loadFromAccessInfo(accessInfo);
            if (wasmRef instanceof AccessBase) {
                throw Error('unexpected error');
            }
            funcRef = wasmRef;
            if (accessInfo instanceof FunctionAccess) {
                // iff top level function, then using call instead of callref
                const parentScope = accessInfo.funcScope.parent;
                if (!(parentScope instanceof ClosureEnvironment)) {
                    funcRef = -1;
                }
            }
        } else {
            funcRef = accessInfo.binaryenRef;
        }

        if (accessInfo instanceof FunctionAccess && funcRef === -1) {
            const { funcScope } = accessInfo;
            if (callWasmArgs.length + 1 < funcScope.paramArray.length) {
                for (
                    let i = callWasmArgs.length + 1;
                    i < funcScope.paramArray.length;
                    i++
                ) {
                    callWasmArgs.push(
                        this.WASMExprGen(
                            funcScope.paramArray[i].initExpression!,
                        ).binaryenRef,
                    );
                }
            }
            callWasmArgs = this._generateInfcArgs(
                paramTypes,
                expr.callArgs,
                callWasmArgs,
            );
            if (!funcScope.isDeclare()) {
                /* Only add context to non-declare functions */
                callWasmArgs = [context, ...callWasmArgs];
            }
            return this.module.call(
                funcScope.mangledName,
                callWasmArgs,
                this.wasmType.getWASMFuncReturnType(funcType),
            );
        } else {
            /* Call closure */
            callWasmArgs = this._generateInfcArgs(
                paramTypes,
                expr.callArgs,
                callWasmArgs,
            );

            /* Extract context and funcref from closure */
            const closureRef = funcRef;
            const closureType =
                binaryenCAPI._BinaryenExpressionGetType(closureRef);
            context = binaryenCAPI._BinaryenStructGet(
                this.module.ptr,
                0,
                closureRef,
                closureType,
                false,
            );
            funcRef = binaryenCAPI._BinaryenStructGet(
                this.module.ptr,
                1,
                closureRef,
                closureType,
                false,
            );
            callWasmArgs.unshift(context);

            return binaryenCAPI._BinaryenCallRef(
                this.module.ptr,
                funcRef,
                arrayToPtr(callWasmArgs).ptr,
                callWasmArgs.length,
                binaryen.getExpressionType(funcRef),
                false,
            );
        }
    }

    /* get callref from class struct vtable index */
    private _generateClassMethodCallRef(
        classRef: binaryen.ExpressionRef | null = null,
        classType: TSClass,
        methodType: TSFunction,
        index: number,
        args: Array<binaryen.ExpressionRef>,
    ) {
        const wasmMethodType = this.wasmType.getWASMType(methodType);
        let vtable: binaryen.ExpressionRef;
        if (classRef) {
            vtable = binaryenCAPI._BinaryenStructGet(
                this.module.ptr,
                0,
                classRef,
                this.wasmType.getWASMClassVtableType(classType),
                false,
            );
        } else {
            vtable = this.wasmType.getWASMClassVtable(classType);
        }
        const targetFunction = binaryenCAPI._BinaryenStructGet(
            this.module.ptr,
            index,
            vtable,
            wasmMethodType,
            false,
        );
        // call object literal method, no @this
        if (methodType.funcKind === FunctionKind.DEFAULT) {
            args = args.filter((item, idx) => idx !== 1);
        }
        return binaryenCAPI._BinaryenCallRef(
            this.module.ptr,
            targetFunction,
            arrayToPtr(args).ptr,
            args.length,
            wasmMethodType,
            false,
        );
    }

    private _generateUnaryExprBlock(
        unaryExpr: UnaryExpression,
        exprRef: binaryen.ExpressionRef,
    ) {
        if (unaryExpr.expressionKind === ts.SyntaxKind.PrefixUnaryExpression) {
            if (
                unaryExpr.operatorKind === ts.SyntaxKind.PlusPlusToken ||
                unaryExpr.operatorKind === ts.SyntaxKind.MinusMinusToken
            ) {
                return this.wasmCompiler.module.block(
                    null,
                    [exprRef, this.WASMExprGen(unaryExpr.operand).binaryenRef],
                    binaryen.f64,
                );
            }
        }
        if (unaryExpr.expressionKind === ts.SyntaxKind.PostfixUnaryExpression) {
            const wasmUnaryOperandExpr = this.WASMExprGen(
                unaryExpr.operand,
            ).binaryenRef;
            if (unaryExpr.operatorKind === ts.SyntaxKind.PlusPlusToken) {
                return this.wasmCompiler.module.block(
                    null,
                    [
                        exprRef,
                        this.module.f64.sub(
                            wasmUnaryOperandExpr,
                            this.module.f64.const(1),
                        ),
                    ],
                    binaryen.f64,
                );
            }
            if (unaryExpr.operatorKind === ts.SyntaxKind.MinusMinusToken) {
                return this.wasmCompiler.module.block(
                    null,
                    [
                        exprRef,
                        this.module.f64.add(
                            wasmUnaryOperandExpr,
                            this.module.f64.const(1),
                        ),
                    ],
                    binaryen.f64,
                );
            }
        }
    }

    private objAssignToInfc(from: Type, to: Type) {
        if (from.kind === TypeKind.CLASS && to.kind === TypeKind.INTERFACE) {
            return true;
        }
        return false;
    }

    private infcAssignToObj(from: Type, to: Type) {
        if (from.kind === TypeKind.INTERFACE && to.kind === TypeKind.CLASS) {
            return true;
        }
        return false;
    }

    private getInfcItable(ref: binaryenCAPI.ExpressionRef) {
        assert(
            binaryen.getExpressionType(ref) === this.wasmType.getInfcTypeRef(),
        );
        const infcItable = binaryenCAPI._BinaryenStructGet(
            this.module.ptr,
            0,
            ref,
            binaryen.i32,
            false,
        );
        return infcItable;
    }

    private getInfcTypeId(ref: binaryenCAPI.ExpressionRef) {
        assert(
            binaryen.getExpressionType(ref) === this.wasmType.getInfcTypeRef(),
        );
        const infcTypeId = binaryenCAPI._BinaryenStructGet(
            this.module.ptr,
            1,
            ref,
            this.wasmType.getInfcTypeRef(),
            false,
        );
        return infcTypeId;
    }

    private getInfcObj(ref: binaryenCAPI.ExpressionRef) {
        assert(
            binaryen.getExpressionType(ref) === this.wasmType.getInfcTypeRef(),
        );
        const infcObj = binaryenCAPI._BinaryenStructGet(
            this.module.ptr,
            2,
            ref,
            binaryen.anyref,
            false,
        );
        return infcObj;
    }

    private objTypeBoxing(ref: binaryen.ExpressionRef, type: TSClass) {
        const itablePtr = this.module.i32.const(
            this.wasmCompiler.generateItable(type),
        );
        const wasmTypeId = this.module.i32.const(type.typeId);
        return binaryenCAPI._BinaryenStructNew(
            this.module.ptr,
            arrayToPtr([itablePtr, wasmTypeId, ref]).ptr,
            3,
            this.wasmType.getInfcHeapTypeRef(),
        );
    }

    private infcTypeUnboxing(ref: binaryen.ExpressionRef, type: Type) {
        assert(type instanceof TSClass);
        const obj = binaryenCAPI._BinaryenStructGet(
            this.module.ptr,
            2,
            ref,
            this.wasmType.getInfcTypeRef(),
            false,
        );
        return binaryenCAPI._BinaryenRefCast(
            this.module.ptr,
            obj,
            this.wasmType.getWASMType(type),
        );
    }

    maybeTypeBoxingAndUnboxing(
        fromType: TSClass,
        toType: TSClass,
        ref: binaryen.ExpressionRef,
    ) {
        if (this.objAssignToInfc(fromType, toType)) {
            return this.objTypeBoxing(ref, fromType);
        }
        if (this.infcAssignToObj(fromType, toType)) {
            const infcTypeId = this.getInfcTypeId(ref);
            const objTypeId = this.module.i32.const(toType.typeId);
            const obj = this.infcTypeUnboxing(ref, toType);
            return createCondBlock(this.module, infcTypeId, objTypeId, obj);
        }
        return ref;
    }

    private dynGetInfcField(
        ref: binaryen.ExpressionRef,
        index: binaryen.ExpressionRef,
        type: Type,
    ) {
        const wasmType = this.wasmType.getWASMType(type);
        const typeKind = type.kind;
        let res: binaryen.ExpressionRef | null = null;
        if (typeKind === TypeKind.BOOLEAN) {
            res = this.module.call(
                structdyn.StructDyn.struct_get_dyn_i32,
                [ref, index],
                binaryen.i32,
            );
        } else if (typeKind === TypeKind.NUMBER) {
            res = this.module.call(
                structdyn.StructDyn.struct_get_dyn_f64,
                [ref, index],
                binaryen.f64,
            );
        } else if (typeKind === TypeKind.FUNCTION) {
            /** get vtable firstly */
            res = this.module.call(
                structdyn.StructDyn.struct_get_dyn_anyref,
                [ref, this.module.i32.const(0)],
                binaryen.anyref,
            );
            res = this.module.call(
                structdyn.StructDyn.struct_get_dyn_funcref,
                [res, index],
                binaryen.funcref,
            );
            res = binaryenCAPI._BinaryenRefCast(this.module.ptr, res, wasmType);
        } else if (wasmType === binaryen.i64) {
            res = this.module.call(
                structdyn.StructDyn.struct_get_dyn_i64,
                [ref, index],
                binaryen.i32,
            );
        } else if (wasmType === binaryen.f32) {
            res = this.module.call(
                structdyn.StructDyn.struct_get_dyn_f32,
                [ref, index],
                binaryen.f32,
            );
        } else {
            const obj = this.module.call(
                structdyn.StructDyn.struct_get_dyn_anyref,
                [ref, index],
                binaryen.anyref,
            );
            res = binaryenCAPI._BinaryenRefCast(this.module.ptr, obj, wasmType);
        }
        if (!res) {
            throw new Error(`get interface field failed, type: ${type}`);
        }
        return res;
    }

    private findItableIndex(
        infcRef: binaryen.ExpressionRef,
        propName: string,
        tag: number,
    ) {
        return this.module.call(
            'find_index',
            [
                this.getInfcItable(infcRef),
                this.module.i32.const(
                    this.wasmCompiler.generateRawString(propName),
                ),
                this.module.i32.const(tag),
            ],
            binaryen.i32,
        );
    }

    private dynSetInfcField(
        ref: binaryen.ExpressionRef,
        index: binaryen.ExpressionRef,
        value: binaryen.ExpressionRef,
        type: Type,
    ) {
        const wasmType = this.wasmType.getWASMType(type);
        const typeKind = type.kind;
        let res: binaryen.ExpressionRef | null = null;

        if (typeKind === TypeKind.BOOLEAN) {
            res = this.module.call(
                structdyn.StructDyn.struct_set_dyn_i32,
                [ref, index, value],
                binaryen.none,
            );
        } else if (typeKind === TypeKind.NUMBER) {
            res = this.module.call(
                structdyn.StructDyn.struct_set_dyn_f64,
                [ref, index, value],
                binaryen.none,
            );
        } else if (typeKind === TypeKind.FUNCTION) {
            res = this.module.call(
                structdyn.StructDyn.struct_get_dyn_anyref,
                [ref, this.module.i32.const(0)],
                binaryen.anyref,
            );
            res = this.module.call(
                structdyn.StructDyn.struct_set_dyn_funcref,
                [res, index, value],
                binaryen.none,
            );
        } else if (wasmType === binaryen.i64) {
            res = this.module.call(
                structdyn.StructDyn.struct_set_dyn_i64,
                [ref, index, value],
                binaryen.none,
            );
        } else if (wasmType === binaryen.f32) {
            res = this.module.call(
                structdyn.StructDyn.struct_set_dyn_f32,
                [ref, index, value],
                binaryen.none,
            );
        } else {
            res = this.module.call(
                structdyn.StructDyn.struct_set_dyn_anyref,
                [ref, index, value],
                binaryen.none,
            );
        }
        if (!res) {
            throw new Error(`set interface field failed, type: ${type}`);
        }
        return res;
    }

    private _getArrayRefLen(
        arrRef: binaryen.ExpressionRef,
    ): binaryen.ExpressionRef {
        const arrLenI32 = binaryenCAPI._BinaryenArrayLen(
            this.module.ptr,
            arrRef,
        );
        const arrLenF64 = this.convertTypeToF64(
            arrLenI32,
            binaryen.getExpressionType(arrLenI32),
        );
        return arrLenF64;
    }

    private _getStringRefLen(
        stringRef: binaryen.ExpressionRef,
    ): binaryen.ExpressionRef {
        const strArray = binaryenCAPI._BinaryenStructGet(
            this.module.ptr,
            1,
            stringRef,
            charArrayTypeInfo.typeRef,
            false,
        );
        const strLenI32 = binaryenCAPI._BinaryenArrayLen(
            this.module.ptr,
            strArray,
        );
        const strLenF64 = this.convertTypeToF64(
            strLenI32,
            binaryen.getExpressionType(strLenI32),
        );
        return strLenF64;
    }

    private parseArguments(type: TSFunction, args: Expression[]) {
        const paramType = type.getParamTypes();
        const res: binaryen.ExpressionRef[] = [];
        let i = 0;
        for (let j = 0; i < args.length && j < paramType.length; i++, j++) {
            if (j === paramType.length - 1 && type.hasRest()) {
                break;
            }
            const value =
                paramType[i].kind === TypeKind.ANY
                    ? this.dynValueGen.WASMDynExprGen(args[i])
                    : this.WASMExprGen(args[i]);
            res.push(value.binaryenRef);
        }
        if (type.hasRest()) {
            const restType = paramType[paramType.length - 1];
            if (restType instanceof TSArray) {
                res.push(this.initArray(restType, args.slice(i)));
            } else {
                Logger.error(`rest type is not array`);
            }
        }

        return res;
    }

    private initArray(arrType: TSArray, elements: Expression[]) {
        const arrayLen = elements.length;
        const array = [];
        if (elements.length === 0) {
            return binaryenCAPI._BinaryenRefNull(
                this.module.ptr,
                binaryenCAPI._BinaryenTypeArrayref(),
            );
        }
        const arrElemType = arrType.elementType;
        for (let i = 0; i < arrayLen; i++) {
            const elemExpr = elements[i];
            let elemExprRef: binaryen.ExpressionRef;
            if (arrType.elementType.kind === TypeKind.ANY) {
                elemExprRef =
                    this.dynValueGen.WASMDynExprGen(elemExpr).binaryenRef;
            } else {
                elemExprRef = this.WASMExprGen(elemExpr).binaryenRef;
                if (
                    arrElemType instanceof TSClass &&
                    elemExpr.exprType instanceof TSClass
                ) {
                    elemExprRef = this.maybeTypeBoxingAndUnboxing(
                        elemExpr.exprType,
                        arrElemType,
                        elemExprRef,
                    );
                }
            }
            array.push(elemExprRef);
        }
        const arrayHeapType = this.wasmType.getWASMHeapType(arrType);
        const arrayValue = binaryenCAPI._BinaryenArrayInit(
            this.module.ptr,
            arrayHeapType,
            arrayToPtr(array).ptr,
            arrayLen,
        );
        return arrayValue;
    }
}

export class WASMDynExpressionGen extends WASMExpressionBase {
    constructor(WASMCompiler: WASMGen) {
        super(WASMCompiler);
    }

    WASMDynExprGen(expr: Expression): WasmValue {
        this.module = this.wasmCompiler.module;
        this.wasmType = this.wasmCompiler.wasmType;
        this.staticValueGen = this.wasmCompiler.wasmExprCompiler;
        this.dynValueGen = this.wasmCompiler.wasmDynExprCompiler;
        this.currentFuncCtx = this.wasmCompiler.curFunctionCtx!;

        let res: binaryen.ExpressionRef;

        switch (expr.expressionKind) {
            case ts.SyntaxKind.NumericLiteral:
            case ts.SyntaxKind.FalseKeyword:
            case ts.SyntaxKind.TrueKeyword:
            case ts.SyntaxKind.StringLiteral:
            case ts.SyntaxKind.NullKeyword:
                res = this.boxBaseTypeToAny(expr);
                break;
            case ts.SyntaxKind.ArrayLiteralExpression:
                res = this.WASMDynArrayExpr(<ArrayLiteralExpression>expr);
                break;
            case ts.SyntaxKind.ObjectLiteralExpression:
                res = this.WASMDynObjExpr(<ObjectLiteralExpression>expr);
                break;
            case ts.SyntaxKind.Identifier:
            case ts.SyntaxKind.BinaryExpression:
            case ts.SyntaxKind.PrefixUnaryExpression:
            case ts.SyntaxKind.PostfixUnaryExpression:
            case ts.SyntaxKind.CallExpression:
            case ts.SyntaxKind.PropertyAccessExpression:
                res = this.boxNonLiteralToAny(expr);
                break;
            default:
                throw new Error('unexpected expr kind ' + expr.expressionKind);
        }

        return {
            binaryenRef: res,
            tsType: expr.exprType,
        };
    }

    private WASMDynArrayExpr(
        expr: ArrayLiteralExpression,
    ): binaryen.ExpressionRef {
        // generate empty any array
        const arrayValue = this.generateDynArray();
        // TODO: generate more array details
        return arrayValue;
    }

    private WASMDynObjExpr(
        expr: ObjectLiteralExpression,
    ): binaryen.ExpressionRef {
        const module = this.module;
        const fields = expr.objectFields;
        const values = expr.objectValues;
        const propertyLen = fields.length;

        // generate empty any obj
        const objValue = this.generateDynObj();
        // add objValue to current scope, push assign statement
        const objLocalVar = this.generateTmpVar('~obj|', 'any');
        const objLocalVarType = objLocalVar.varType;
        const objLocalVarWasmType = this.wasmType.getWASMType(objLocalVarType);
        this.currentFuncCtx.insert(
            this.setVariableToCurrentScope(objLocalVar, objValue),
        );
        // set obj's properties
        for (let i = 0; i < propertyLen; i++) {
            const propNameExpr = fields[i];
            const propNameExprRef = module.i32.const(
                this.wasmCompiler.generateRawString(
                    propNameExpr.identifierName,
                ),
            );
            const propValueExpr = values[i];
            const propValueExprRef =
                this.WASMDynExprGen(propValueExpr).binaryenRef;
            const setPropertyExpression = module.drop(
                module.call(
                    dyntype.dyntype_set_property,
                    [
                        module.global.get(
                            dyntype.dyntype_context,
                            dyntype.dyn_ctx_t,
                        ),
                        this.getLocalValue(
                            objLocalVar.varIndex,
                            objLocalVarWasmType,
                        ),
                        propNameExprRef,
                        propValueExprRef,
                    ],
                    dyntype.int,
                ),
            );
            this.currentFuncCtx.insert(setPropertyExpression);
        }
        return this.getVariableValue(objLocalVar, objLocalVarWasmType);
    }
}
