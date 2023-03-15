/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function callReturnTest(a = 10, b = 1, c = 99) {
    return a + b + c;
}
export function callInternalReturnTest(a: number, b = 2) {
    const c = callReturnTest(a, b, 3);
    const d = callReturnTest();
    return c + d;
}
