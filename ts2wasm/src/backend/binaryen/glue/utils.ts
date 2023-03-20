/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import binaryen from 'binaryen';
import * as binaryenCAPI from './binaryen.js';

export const STRING_LENGTH_FUNC = 'lib-string-length';
export const STRING_CONCAT_FUNC = 'lib-string-concat';
export const STRING_SLICE_FUNC = 'lib-string-slice';
export const GLOBAL_INIT_FUNC = 'global_init';

export interface ptrInfo {
    ptr: number;
    len: number;
}

export interface typeInfo {
    typeRef: binaryenCAPI.TypeRef;
    heapTypeRef: binaryenCAPI.HeapTypeRef;
}

export interface FlattenLoop {
    label: string;
    condition: binaryen.ExpressionRef;
    statements: binaryen.ExpressionRef;
    incrementor?: binaryen.ExpressionRef;
}

export interface IfStatementInfo {
    condition: binaryen.ExpressionRef;
    ifTrue: binaryen.ExpressionRef;
    ifFalse: binaryen.ExpressionRef;
}
