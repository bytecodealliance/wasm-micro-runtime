/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

function callReturnTest1(a = 10, b = 1, c = 99) {
    return a + b + c;
}

function callReturnTest2(a: number, b = 2) {
    const c = a + b;
    return c;
}

function callReturnTest3(a: number, b: number, c: number) {
    return a + b + c;
}

export function noDefaultParam() {
    return callReturnTest3(1, 2, 3);
}

export function allDefaultParam() {
    const a = callReturnTest1(1, 2, 3);
    return a;
}

export function someDefaultParam() {
    return callReturnTest2(2, 3);
}

function outer(a: any) {
    function inner(b: number) {
        return (a as number) + b;
    }
    return inner;
}

export function paramIsAny() {
    const res = outer(1)(2);
    return res;
}
