/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function newArrayNumber() {
    const array1 = new Array<number>(1);
    array1[0] = 3;
    return array1[0];
}

export function newArrayNumberWithParam(n: number) {
    const array1 = new Array<Array<number>>(n);
    array1[0] = new Array<number>(n);
    array1[0][0] = n;
    return array1[0][0];
}
