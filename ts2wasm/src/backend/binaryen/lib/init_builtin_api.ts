/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import binaryen from 'binaryen';
import ts from 'typescript';
import * as binaryenCAPI from '../glue/binaryen.js';
import { BuiltinNames } from '../../../../lib/builtin/builtin_name.js';
import { emptyStructType } from '../glue/transform.js';
import { flattenLoopStatement, FlattenLoop } from '../utils.js';
import { dyntype } from './dyntype/utils.js';
import { arrayToPtr } from '../glue/transform.js';
import {
    charArrayTypeInfo,
    stringArrayTypeInfo,
    stringTypeInfo,
} from '../glue/packType.js';

function getFuncName(moduleName: string, funcName: string, delimiter = '|') {
    return moduleName.concat(delimiter).concat(funcName);
}

function string_concat(module: binaryen.Module) {
    /** Args: context, this, string[] */
    const thisStrStructIdx = 1;
    const paramStrArrayIdx = 2;
    /** Locals: totalLen, for_i(i32), newStrArrayIdx(char_array), copyCurLenIdx(i32) */
    const totalLenIdx = 3;
    const for_i_Idx = 4;
    const newStrArrayIdx = 5;
    const copyCurLenIdx = 6;
    /** structure index information */
    const arrayIdxInStruct = 1;
    const thisStrStruct = module.local.get(
        thisStrStructIdx,
        stringTypeInfo.typeRef,
    );
    const paramStrArray = module.local.get(
        paramStrArrayIdx,
        stringArrayTypeInfo.typeRef,
    );
    const thisStrArray = binaryenCAPI._BinaryenStructGet(
        module.ptr,
        arrayIdxInStruct,
        thisStrStruct,
        charArrayTypeInfo.typeRef,
        false,
    );
    const thisStrLen = binaryenCAPI._BinaryenArrayLen(module.ptr, thisStrArray);
    const paramStrArrayLen = binaryenCAPI._BinaryenArrayLen(
        module.ptr,
        paramStrArray,
    );

    const getStringArrayFromRestParams = (module: binaryen.Module) => {
        return binaryenCAPI._BinaryenStructGet(
            module.ptr,
            arrayIdxInStruct,
            binaryenCAPI._BinaryenArrayGet(
                module.ptr,
                module.local.get(paramStrArrayIdx, stringArrayTypeInfo.typeRef),
                module.local.get(for_i_Idx, binaryen.i32),
                stringTypeInfo.typeRef,
                false,
            ),
            charArrayTypeInfo.typeRef,
            false,
        );
    };

    const statementArray: binaryen.ExpressionRef[] = [];
    /** 1. get total str length */
    statementArray.push(module.local.set(totalLenIdx, thisStrLen));
    const for_label_1 = 'for_loop_1_block';
    const for_init_1 = module.local.set(for_i_Idx, module.i32.const(0));
    const for_condition_1 = module.i32.lt_u(
        module.local.get(for_i_Idx, binaryen.i32),
        paramStrArrayLen,
    );
    const for_incrementor_1 = module.local.set(
        for_i_Idx,
        module.i32.add(
            module.local.get(for_i_Idx, binaryen.i32),
            module.i32.const(1),
        ),
    );
    const for_body_1 = module.local.set(
        totalLenIdx,
        module.i32.add(
            module.local.get(totalLenIdx, binaryen.i32),
            binaryenCAPI._BinaryenArrayLen(
                module.ptr,
                getStringArrayFromRestParams(module),
            ),
        ),
    );

    const flattenLoop_1: FlattenLoop = {
        label: for_label_1,
        condition: for_condition_1,
        statements: for_body_1,
        incrementor: for_incrementor_1,
    };
    statementArray.push(for_init_1);
    statementArray.push(
        module.loop(
            for_label_1,
            flattenLoopStatement(
                flattenLoop_1,
                ts.SyntaxKind.ForStatement,
                module,
            ),
        ),
    );

    /** 2. generate new string */
    statementArray.push(
        module.local.set(
            newStrArrayIdx,
            binaryenCAPI._BinaryenArrayNew(
                module.ptr,
                charArrayTypeInfo.heapTypeRef,
                module.local.get(totalLenIdx, binaryen.i32),
                module.i32.const(0),
            ),
        ),
    );

    /** 3. traverse paramStrArray, do copy */
    statementArray.push(
        binaryenCAPI._BinaryenArrayCopy(
            module.ptr,
            module.local.get(newStrArrayIdx, charArrayTypeInfo.typeRef),
            module.i32.const(0),
            thisStrArray,
            module.i32.const(0),
            thisStrLen,
        ),
    );
    statementArray.push(module.local.set(copyCurLenIdx, thisStrLen));

    const for_label_2 = 'for_loop_2_block';
    const for_init_2 = module.local.set(for_i_Idx, module.i32.const(0));
    const for_condition_2 = module.i32.lt_u(
        module.local.get(for_i_Idx, binaryen.i32),
        paramStrArrayLen,
    );
    const for_incrementor_2 = module.local.set(
        for_i_Idx,
        module.i32.add(
            module.local.get(for_i_Idx, binaryen.i32),
            module.i32.const(1),
        ),
    );
    const for_body_2 = module.block(null, [
        binaryenCAPI._BinaryenArrayCopy(
            module.ptr,
            module.local.get(newStrArrayIdx, charArrayTypeInfo.typeRef),
            module.local.get(copyCurLenIdx, binaryen.i32),
            getStringArrayFromRestParams(module),
            module.i32.const(0),
            binaryenCAPI._BinaryenArrayLen(
                module.ptr,
                getStringArrayFromRestParams(module),
            ),
        ),
        module.local.set(
            copyCurLenIdx,
            module.i32.add(
                module.local.get(copyCurLenIdx, binaryen.i32),
                binaryenCAPI._BinaryenArrayLen(
                    module.ptr,
                    getStringArrayFromRestParams(module),
                ),
            ),
        ),
    ]);

    const flattenLoop_2: FlattenLoop = {
        label: for_label_2,
        condition: for_condition_2,
        statements: for_body_2,
        incrementor: for_incrementor_2,
    };
    statementArray.push(for_init_2);
    statementArray.push(
        module.loop(
            for_label_2,
            flattenLoopStatement(
                flattenLoop_2,
                ts.SyntaxKind.ForStatement,
                module,
            ),
        ),
    );

    /** 4. generate new string structure */
    statementArray.push(
        module.return(
            binaryenCAPI._BinaryenStructNew(
                module.ptr,
                arrayToPtr([
                    module.i32.const(0),
                    module.local.get(newStrArrayIdx, charArrayTypeInfo.typeRef),
                ]).ptr,
                2,
                stringTypeInfo.heapTypeRef,
            ),
        ),
    );

    /** 5. generate block, return block */
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

    statementArray.push(setDefault);
    statementArray.push(is_any_array);
    statementArray.push(returnStmt);

    return module.block(null, statementArray);
}

export function callBuiltInAPIs(module: binaryen.Module) {
    /** Math.sqrt */
    module.addFunction(
        getFuncName(
            BuiltinNames.builtinModuleName,
            BuiltinNames.mathSqrtFuncName,
        ),
        binaryen.createType([emptyStructType.typeRef, binaryen.f64]),
        binaryen.f64,
        [],
        module.f64.sqrt(module.local.get(1, binaryen.f64)),
    );
    /** Math.abs */
    module.addFunction(
        getFuncName(
            BuiltinNames.builtinModuleName,
            BuiltinNames.mathAbsFuncName,
        ),
        binaryen.createType([emptyStructType.typeRef, binaryen.f64]),
        binaryen.f64,
        [],
        module.f64.abs(module.local.get(1, binaryen.f64)),
    );
    /** Math.ceil */
    module.addFunction(
        getFuncName(
            BuiltinNames.builtinModuleName,
            BuiltinNames.mathCeilFuncName,
        ),
        binaryen.createType([emptyStructType.typeRef, binaryen.f64]),
        binaryen.f64,
        [],
        module.f64.ceil(module.local.get(1, binaryen.f64)),
    );
    /** Math.floor */
    module.addFunction(
        getFuncName(
            BuiltinNames.builtinModuleName,
            BuiltinNames.mathFloorFuncName,
        ),
        binaryen.createType([emptyStructType.typeRef, binaryen.f64]),
        binaryen.f64,
        [],
        module.f64.floor(module.local.get(1, binaryen.f64)),
    );
    /** Math.trunc */
    module.addFunction(
        getFuncName(
            BuiltinNames.builtinModuleName,
            BuiltinNames.mathTruncFuncName,
        ),
        binaryen.createType([emptyStructType.typeRef, binaryen.f64]),
        binaryen.f64,
        [],
        module.f64.trunc(module.local.get(1, binaryen.f64)),
    );
    /** Array.isArray */
    module.addFunction(
        getFuncName(
            BuiltinNames.builtinModuleName,
            BuiltinNames.arrayIsArrayFuncName,
        ),
        binaryen.createType([emptyStructType.typeRef, binaryen.anyref]),
        binaryen.i32,
        [binaryen.i32],
        Array_isArray(module),
    );
    /** string */
    module.addFunction(
        getFuncName(
            BuiltinNames.builtinModuleName,
            BuiltinNames.stringConcatFuncName,
        ),
        binaryen.createType([
            emptyStructType.typeRef,
            stringTypeInfo.typeRef,
            stringArrayTypeInfo.typeRef,
        ]),
        stringTypeInfo.typeRef,
        [binaryen.i32, binaryen.i32, charArrayTypeInfo.typeRef, binaryen.i32],
        string_concat(module),
    );
    module.addFunction(
        getFuncName(
            BuiltinNames.builtinModuleName,
            BuiltinNames.stringSliceFuncName,
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
