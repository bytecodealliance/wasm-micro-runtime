/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function ifElse() {
    const a = 10;
    const b = 5;
    const c = 1;
    let d = 1;
    if (a > 1) {
        d = 10;
    } else {
        d = 100;
    }
    return a + b + c + d;
}
// 26

export function nestedIf() {
    let a = 10;
    const b = 5;
    const c = 1;
    let d = 1;
    if (a > 1) {
        d = 10;
        if (b > 2) {
            a = d + 10;
        }
    } else {
        d = 100;
    }
    return a + b + c + d;
}
// 36

export function noElseBranch() {
    const a = 10;
    const b = 5;
    const c = 1;
    const d = 1;
    // eslint-disable-next-line no-empty
    if (a > 1) {
    }
    return a + b + c + d;
}
// 17

class A {
    //
}

export function returnInIf(x: number) {
    const res: A | null = null;
    if (!res) {
        if (x) {
            return x;
        }
    }
    return -1;
}
