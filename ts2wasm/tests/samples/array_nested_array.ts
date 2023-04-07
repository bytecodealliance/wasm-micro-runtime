/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function nestedArray() {
    const array1: Array<string[]> = new Array<string[]>(1);
    array1[0][0] = 'hi';
    return array1;
}
