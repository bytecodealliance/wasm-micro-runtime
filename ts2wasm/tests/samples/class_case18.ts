/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A {
    a: number;
    b: number;
    static c = 11;
    static readonly d = 12 + A.c;
    constructor(a: number, b: number) {
        this.a = a;
        this.b = b;
    }
}
export function classTest() {
    return A.d + A.c;
}

// 34
