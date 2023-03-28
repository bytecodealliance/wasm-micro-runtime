/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function arrayTest12() {
    let array1: number[];
    array1 = new Array<number>(2);
    array1[0] = 3;
    array1[1] = 5;
    // return array1;
    return array1[0];
}
