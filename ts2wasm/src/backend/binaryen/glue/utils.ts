/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import * as binaryenCAPI from './binaryen.js';

export const GLOBAL_INIT_FUNC = 'global_init';

export interface ptrInfo {
    ptr: number;
    len: number;
}

export interface typeInfo {
    typeRef: binaryenCAPI.TypeRef;
    heapTypeRef: binaryenCAPI.HeapTypeRef;
}
