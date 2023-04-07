/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

function callReturnTest(a = 10, b = 1, c = 99) {
    return a + b + c;
}

export function getValueWithDefaultParam() {
    return callReturnTest() + callReturnTest(1, 2, 3);
}

export function callInnerFunc(a: number, b = 2) {
    function callReturnTest(a: number, b: number, c: number) {
        return a + b + c;
    }
    const c = callReturnTest(a, b, 3);
    const d = callReturnTest(1, 2, 3);
    return c + d;
}

export function recursive(n: number): number {
    if (n == 1) {
        return 1;
    }
    if (n == 2) {
        return 1;
    }
    return recursive(n - 2) + recursive(n - 1);
}
