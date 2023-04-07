/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function addAnyAny() {
    let a: any = 1;
    let b: any = 2;
    let c: any = a + b;
    return c;
}

export function addNumberAnyInBinaryExpr() {
    let a: any = 1;
    let b: any;
    b = a + 1;
    return b;
}

export function addNumberAnyInMulExpr() {
    let a: any = 1;
    let c: any;
    c = 2 + a + 6;
    return c;
}
