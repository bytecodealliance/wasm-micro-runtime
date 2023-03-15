/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function arrayTest11() {
    const array1: number[] = new Array<number>(2);
    array1[0] = 3;
    array1[1] = 5;
    return array1;
}
