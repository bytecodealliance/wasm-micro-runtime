/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import binaryen from 'binaryen';
import * as binaryenCAPI from './binaryen.js';
import { ptrInfo, typeInfo } from './utils.js';

/** type system */
export const isorecursive: binaryenCAPI.TypeSystem = 0;
export const nominal: binaryenCAPI.TypeSystem = 1;

/** value types */
export namespace BinaryenType {
    // value types
    // _BinaryenTypeInt32
    export const I32: binaryen.Type = 2;
    // _BinaryenTypeInt64
    export const I64: binaryen.Type = 3;
    // _BinaryenTypeFloat32
    export const F32: binaryen.Type = 4;
    // _BinaryenTypeFloat64
    export const F64: binaryen.Type = 5;
}

/** packed struct field types */
export namespace Pakced {
    // _BinaryenPackedTypeNotPacked
    export const Not: binaryenCAPI.PackedType = 0;
    // _BinaryenPackedTypeInt8
    export const I8: binaryenCAPI.PackedType = 1;
    // _BinaryenPackedTypeInt16
    export const I16: binaryenCAPI.PackedType = 2;
}

export function arrayToPtr(array: number[]): ptrInfo {
    const ptrInfo: ptrInfo = {
        ptr: 0,
        len: 0,
    };
    if (!array) ptrInfo;
    const len = array.length;
    const ptrAddress = binaryenCAPI._malloc(len << 2);
    let idx = ptrAddress;
    for (let i = 0; i < len; ++i) {
        const val = array[i];
        binaryenCAPI.__i32_store(idx, <number>val);
        idx += 4;
    }
    ptrInfo.ptr = ptrAddress;
    ptrInfo.len = len;
    return ptrInfo;
}

function allocU32Array(u32s: binaryenCAPI.u32[] | null): binaryenCAPI.usize {
    if (!u32s) return 0;
    const len = u32s.length;
    const ptr = binaryenCAPI._malloc(len << 2);
    let idx = ptr;
    for (let i = 0; i < len; ++i) {
        binaryenCAPI.__i32_store(idx, u32s[i]);
        idx += 4;
    }
    return ptr;
}

function allocU8Array(u8s: boolean[] | null): binaryenCAPI.usize {
    if (!u8s) return 0;
    const len = u8s.length;
    const ptr = binaryenCAPI._malloc(len);
    for (let i = 0; i < len; ++i) {
        const value = u8s[i] ? 1 : 0;
        binaryenCAPI.__i32_store8(ptr + i, value);
    }
    return ptr;
}

export function ptrToArray(ptrInfo: ptrInfo): number[] {
    if (!ptrInfo) return [];
    const ptr = ptrInfo.ptr;
    const len = ptrInfo.len;
    const array = [];
    let idx = ptr;
    for (let i = 0; i < len; ++i) {
        const val = binaryenCAPI.__i32_load(idx);
        array.push(val);
        idx += 4;
    }
    binaryenCAPI._free(ptr);
    return array;
}

export function i32(
    module: binaryen.Module,
    alloc: binaryenCAPI.usize,
    value: binaryenCAPI.i32,
): binaryen.ExpressionRef {
    binaryenCAPI._BinaryenLiteralInt32(alloc, value);
    return binaryenCAPI._BinaryenConst(module.ptr, alloc);
}

export function initArrayType(
    elementType: binaryenCAPI.TypeRef,
    elementPackedType: binaryenCAPI.PackedType,
    elementMutable: binaryenCAPI.bool,
    nullable: binaryenCAPI.bool,
): typeInfo {
    const tb: binaryenCAPI.TypeBuilderRef = binaryenCAPI._TypeBuilderCreate(1);
    binaryenCAPI._TypeBuilderSetArrayType(
        tb,
        0,
        elementType,
        elementPackedType,
        elementMutable,
    );
    const builtHeapType: binaryenCAPI.HeapTypeRef[] = new Array(1);
    const builtHeapTypePtr = arrayToPtr(builtHeapType);
    binaryenCAPI._TypeBuilderBuildAndDispose(tb, builtHeapTypePtr.ptr, 0, 0);
    const arrayType = binaryenCAPI._BinaryenTypeFromHeapType(
        ptrToArray(builtHeapTypePtr)[0],
        nullable,
    );
    const arrayRef = binaryenCAPI._BinaryenTypeGetHeapType(arrayType);
    const arrayTypeInfo: typeInfo = {
        typeRef: arrayType,
        heapTypeRef: arrayRef,
    };
    return arrayTypeInfo;
}

export const emptyStructType = initStructType([], [], [], 0, true);

export function initStructType(
    fieldTypesList: Array<binaryenCAPI.TypeRef>,
    fieldPackedTypesList: Array<binaryenCAPI.PackedType>,
    fieldMutablesList: Array<boolean>,
    numFields: binaryenCAPI.i32,
    nullable: binaryenCAPI.bool,
    baseType?: binaryenCAPI.HeapTypeRef,
): typeInfo {
    const fieldTypes = arrayToPtr(fieldTypesList).ptr;
    const fieldPackedTypes = allocU32Array(fieldPackedTypesList);
    const fieldMutables = allocU8Array(fieldMutablesList);
    const tb: binaryenCAPI.TypeBuilderRef = binaryenCAPI._TypeBuilderCreate(1);
    binaryenCAPI._TypeBuilderSetStructType(
        tb,
        0,
        fieldTypes,
        fieldPackedTypes,
        fieldMutables,
        numFields,
    );
    if (fieldTypesList.length > 0) {
        const subType = baseType ? baseType : emptyStructType.heapTypeRef;
        binaryenCAPI._TypeBuilderSetSubType(tb, 0, subType);
    }
    const builtHeapType: binaryenCAPI.HeapTypeRef[] = new Array(1);
    const builtHeapTypePtr = arrayToPtr(builtHeapType);
    binaryenCAPI._TypeBuilderBuildAndDispose(tb, builtHeapTypePtr.ptr, 0, 0);
    const structType = binaryenCAPI._BinaryenTypeFromHeapType(
        ptrToArray(builtHeapTypePtr)[0],
        nullable,
    );
    const structRef = binaryenCAPI._BinaryenTypeGetHeapType(structType);
    const structTypeInfo: typeInfo = {
        typeRef: structType,
        heapTypeRef: structRef,
    };
    return structTypeInfo;
}

export const charArrayTypeInformation = genarateCharArrayTypeInfo();
export const stringTypeInformation = generateStringTypeInfo();
export const numberArrayTypeInformation = genarateNumberArrayTypeInfo();
export const stringArrayTypeInformation = genarateStringArrayTypeInfo();
export const boolArrayTypeInformation = genarateBoolArrayTypeInfo();
export const anyArrayTypeInformation = genarateAnyArrayTypeInfo();
export const objectStructTypeInformation = emptyStructType;
export const infcTypeInformation = generateInfcTypeInfo();

// generate array type to store character context
function genarateCharArrayTypeInfo(): typeInfo {
    const charArrayTypeInfo = initArrayType(
        BinaryenType.I32,
        Pakced.I8,
        true,
        true,
    );
    return charArrayTypeInfo;
}

// generate struct type to store string information
function generateStringTypeInfo(): typeInfo {
    const charArrayTypeInfo = charArrayTypeInformation;
    const stringTypeInfo = initStructType(
        [
            BinaryenType.I32,
            binaryenCAPI._BinaryenTypeFromHeapType(
                charArrayTypeInfo.heapTypeRef,
                true,
            ),
        ],
        [Pakced.Not, Pakced.Not],
        [true, true],
        2,
        true,
    );
    return stringTypeInfo;
}

// generate number array type
function genarateNumberArrayTypeInfo(): typeInfo {
    const numberArrayTypeInfo = initArrayType(
        BinaryenType.F64,
        Pakced.Not,
        true,
        true,
    );
    return numberArrayTypeInfo;
}

// generate string array type
function genarateStringArrayTypeInfo(): typeInfo {
    const stringTypeInfo = stringTypeInformation;
    const stringArrayTypeInfo = initArrayType(
        stringTypeInfo.typeRef,
        Pakced.Not,
        true,
        true,
    );
    return stringArrayTypeInfo;
}

function generateInfcTypeInfo(): typeInfo {
    return initStructType(
        [binaryen.i32, binaryen.i32, binaryen.anyref],
        [Pakced.Not, Pakced.Not, Pakced.Not],
        [false, false, true],
        3,
        true,
    );
}

// generate bool array type
function genarateBoolArrayTypeInfo(): typeInfo {
    const boolArrayTypeInfo = initArrayType(
        BinaryenType.I32,
        Pakced.Not,
        true,
        true,
    );
    return boolArrayTypeInfo;
}

// generate any array type
function genarateAnyArrayTypeInfo(): typeInfo {
    const anyArrayTypeInfo = initArrayType(
        binaryenCAPI._BinaryenTypeAnyref(),
        Pakced.Not,
        true,
        true,
    );
    return anyArrayTypeInfo;
}

export function createSignatureTypeRefAndHeapTypeRef(
    parameterTypes: Array<binaryenCAPI.TypeRef>,
    returnType: binaryenCAPI.TypeRef,
): typeInfo {
    const parameterLen = parameterTypes.length;
    const builder = binaryenCAPI._TypeBuilderCreate(1);
    const tempSignatureIndex = 0;
    let tempParamTypes = !parameterLen ? binaryen.none : parameterTypes[0];
    if (parameterLen > 1) {
        const tempPtr = arrayToPtr(parameterTypes).ptr;
        tempParamTypes = binaryenCAPI._TypeBuilderGetTempTupleType(
            builder,
            tempPtr,
            parameterLen,
        );
        binaryenCAPI._free(tempPtr);
    }
    binaryenCAPI._TypeBuilderSetSignatureType(
        builder,
        tempSignatureIndex,
        tempParamTypes,
        returnType,
    );
    const builtHeapType: binaryenCAPI.HeapTypeRef[] = new Array(1);
    const builtHeapTypePtr = arrayToPtr(builtHeapType);
    binaryenCAPI._TypeBuilderBuildAndDispose(
        builder,
        builtHeapTypePtr.ptr,
        0,
        0,
    );
    const signatureType = binaryenCAPI._BinaryenTypeFromHeapType(
        ptrToArray(builtHeapTypePtr)[0],
        true,
    );
    const signatureHeapType =
        binaryenCAPI._BinaryenTypeGetHeapType(signatureType);
    const signature: typeInfo = {
        typeRef: signatureType,
        heapTypeRef: signatureHeapType,
    };
    return signature;
}

export function createCondBlock(
    module: binaryen.Module,
    l: binaryen.ExpressionRef,
    r: binaryen.ExpressionRef,
    ifTrue: binaryen.ExpressionRef,
    ifFalse: binaryen.ExpressionRef = module.unreachable(),
): binaryen.ExpressionRef {
    const cond = module.if(module.i32.eq(l, r), ifTrue, ifFalse);
    const resType = binaryen.getExpressionType(ifTrue);
    const condBlock = module.block(null, [cond], resType);
    return condBlock;
}
