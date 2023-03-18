/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import binaryen from 'binaryen';
import * as binaryenCAPI from '../../src/backend/binaryen/glue/binaryen.js';
import { BuiltinNames } from './builtinUtil.js';
import { generateWatFile, getFuncName } from './utils.js';
import { emptyStructType } from '../../src/backend/binaryen/glue/transform.js';
import { dyntype } from '../dyntype/utils.js';
import {
    importAnyLibAPI,
    generateGlobalContext,
    generateInitDynContext,
    generateFreeDynContext,
} from '../envInit.js';
import { arrayToPtr } from '../../src/backend/binaryen/glue/transform.js';
import {
    charArrayTypeInfo,
    stringTypeInfo,
} from '../../src/backend/binaryen/glue/packType.js';

function string_concat(module: binaryen.Module) {
    const strStruct1 = module.local.get(1, stringTypeInfo.typeRef);
    const strStruct2 = module.local.get(2, stringTypeInfo.typeRef);
    const strArray1 = binaryenCAPI._BinaryenStructGet(
        module.ptr,
        1,
        strStruct1,
        charArrayTypeInfo.typeRef,
        false,
    );
    const strArray2 = binaryenCAPI._BinaryenStructGet(
        module.ptr,
        1,
        strStruct2,
        charArrayTypeInfo.typeRef,
        false,
    );
    const str1Len = binaryenCAPI._BinaryenArrayLen(module.ptr, strArray1);
    const str2Len = binaryenCAPI._BinaryenArrayLen(module.ptr, strArray2);
    const statementArray: binaryen.ExpressionRef[] = [];
    const newStrLen = module.i32.add(str1Len, str2Len);
    const newStrArrayIndex = 3;
    const newStrArrayType = charArrayTypeInfo.typeRef;
    const newStrArrayStatement = module.local.set(
        newStrArrayIndex,
        binaryenCAPI._BinaryenArrayNew(
            module.ptr,
            charArrayTypeInfo.heapTypeRef,
            newStrLen,
            module.i32.const(0),
        ),
    );
    const arrayCopyStatement1 = binaryenCAPI._BinaryenArrayCopy(
        module.ptr,
        module.local.get(newStrArrayIndex, newStrArrayType),
        module.i32.const(0),
        strArray1,
        module.i32.const(0),
        str1Len,
    );
    const arrayCopyStatement2 = binaryenCAPI._BinaryenArrayCopy(
        module.ptr,
        module.local.get(newStrArrayIndex, newStrArrayType),
        str1Len,
        strArray2,
        module.i32.const(0),
        str2Len,
    );
    const newStrStruct = binaryenCAPI._BinaryenStructNew(
        module.ptr,
        arrayToPtr([
            module.i32.const(0),
            module.local.get(newStrArrayIndex, newStrArrayType),
        ]).ptr,
        2,
        stringTypeInfo.heapTypeRef,
    );
    statementArray.push(newStrArrayStatement);
    statementArray.push(arrayCopyStatement1);
    statementArray.push(arrayCopyStatement2);
    statementArray.push(module.return(newStrStruct));
    const concatBlock = module.block('concat', statementArray);
    return concatBlock;
}

function string_slice(module: binaryen.Module) {
    const context = module.local.get(0, emptyStructType.typeRef);
    const strStruct = module.local.get(1, stringTypeInfo.typeRef);
    const start = module.local.get(2, binaryen.i32);
    const end = module.local.get(3, binaryen.i32);
    const strArray = binaryenCAPI._BinaryenStructGet(
        module.ptr,
        1,
        strStruct,
        charArrayTypeInfo.typeRef,
        false,
    );
    const newStrLen = module.i32.sub(end, start);
    const statementArray: binaryen.ExpressionRef[] = [];
    const newStrArrayIndex = 4;
    const newStrArrayType = charArrayTypeInfo.typeRef;
    const newStrArrayStatement = module.local.set(
        newStrArrayIndex,
        binaryenCAPI._BinaryenArrayNew(
            module.ptr,
            charArrayTypeInfo.heapTypeRef,
            newStrLen,
            module.i32.const(0),
        ),
    );
    const arrayCopyStatement = binaryenCAPI._BinaryenArrayCopy(
        module.ptr,
        module.local.get(newStrArrayIndex, newStrArrayType),
        module.i32.const(0),
        strArray,
        start,
        newStrLen,
    );
    const newStrStruct = binaryenCAPI._BinaryenStructNew(
        module.ptr,
        arrayToPtr([
            module.i32.const(0),
            module.local.get(newStrArrayIndex, newStrArrayType),
        ]).ptr,
        2,
        stringTypeInfo.heapTypeRef,
    );
    statementArray.push(newStrArrayStatement);
    statementArray.push(arrayCopyStatement);
    statementArray.push(module.return(newStrStruct));
    const sliceBlock = module.block('slice', statementArray);
    return sliceBlock;
}

function Array_isArray(module: binaryen.Module) {
    const param = module.local.get(1, binaryen.anyref);
    const statementArray: binaryen.ExpressionRef[] = [];

    const setDefault = module.local.set(2, module.i32.const(0));
    const setTrue = module.local.set(2, module.i32.const(1));
    const returnStmt = module.return(module.local.get(2, binaryen.i32));

    const dynTypeIsArray = module.call(
        dyntype.dyntype_is_array,
        [module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t), param],
        dyntype.bool,
    );
    const is_any_array = module.if(
        module.i32.eq(dynTypeIsArray, dyntype.bool_true),
        setTrue,
    );

    generateGlobalContext(module);
    statementArray.push(generateInitDynContext(module));
    statementArray.push(setDefault);
    statementArray.push(is_any_array);
    statementArray.push(generateFreeDynContext(module));
    statementArray.push(returnStmt);

    return module.block(null, statementArray);
}

export function callBuiltInAPIs(module: binaryen.Module) {
    /** init lib env */
    importAnyLibAPI(module);
    /** Math.sqrt */
    module.addFunction(
        getFuncName(
            BuiltinNames.bulitIn_module_name,
            BuiltinNames.Math_sqrt_funcName,
        ),
        binaryen.createType([emptyStructType.typeRef, binaryen.f64]),
        binaryen.f64,
        [],
        module.f64.sqrt(module.local.get(1, binaryen.f64)),
    );
    /** Math.abs */
    module.addFunction(
        getFuncName(
            BuiltinNames.bulitIn_module_name,
            BuiltinNames.Math_abs_funcName,
        ),
        binaryen.createType([emptyStructType.typeRef, binaryen.f64]),
        binaryen.f64,
        [],
        module.f64.abs(module.local.get(1, binaryen.f64)),
    );
    /** Math.ceil */
    module.addFunction(
        getFuncName(
            BuiltinNames.bulitIn_module_name,
            BuiltinNames.Math_ceil_funcName,
        ),
        binaryen.createType([emptyStructType.typeRef, binaryen.f64]),
        binaryen.f64,
        [],
        module.f64.ceil(module.local.get(1, binaryen.f64)),
    );
    /** Math.floor */
    module.addFunction(
        getFuncName(
            BuiltinNames.bulitIn_module_name,
            BuiltinNames.Math_floor_funcName,
        ),
        binaryen.createType([emptyStructType.typeRef, binaryen.f64]),
        binaryen.f64,
        [],
        module.f64.floor(module.local.get(1, binaryen.f64)),
    );
    /** Math.trunc */
    module.addFunction(
        getFuncName(
            BuiltinNames.bulitIn_module_name,
            BuiltinNames.Math_trunc_funcName,
        ),
        binaryen.createType([emptyStructType.typeRef, binaryen.f64]),
        binaryen.f64,
        [],
        module.f64.trunc(module.local.get(1, binaryen.f64)),
    );
    /** Array.isArray */
    module.addFunction(
        getFuncName(
            BuiltinNames.bulitIn_module_name,
            BuiltinNames.Array_isArray_funcName,
        ),
        binaryen.createType([emptyStructType.typeRef, binaryen.anyref]),
        binaryen.i32,
        [binaryen.i32],
        Array_isArray(module),
    );
    /** string */
    module.addFunction(
        getFuncName(
            BuiltinNames.bulitIn_module_name,
            BuiltinNames.string_concat_funcName,
        ),
        binaryen.createType([
            emptyStructType.typeRef,
            stringTypeInfo.typeRef,
            stringTypeInfo.typeRef,
        ]),
        stringTypeInfo.typeRef,
        [charArrayTypeInfo.typeRef],
        string_concat(module),
    );
    module.addFunction(
        getFuncName(
            BuiltinNames.bulitIn_module_name,
            BuiltinNames.string_slice_funcName,
        ),
        binaryen.createType([
            emptyStructType.typeRef,
            stringTypeInfo.typeRef,
            binaryen.i32,
            binaryen.i32,
        ]),
        stringTypeInfo.typeRef,
        [charArrayTypeInfo.typeRef],
        string_slice(module),
    );
    /** TODO: */
    /** array */
}

export function initBuiltInAPIs() {
    const module = new binaryen.Module();
    callBuiltInAPIs(module);
    generateWatFile(module, 'API.wat');
    module.dispose();
}

initBuiltInAPIs();
