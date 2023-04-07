/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A11 {
    a: number = 10;
    constructor(a1: number) {
        this.a = a1;
        this.b = true;
    }
    b = false;
    c = 'c';
}

export function withCtor() {
    let a: A11 = new A11(18);
    return a.a;
}

class A12 {
    a = 10;
    b = false;
    c = 'c';
}

export function withoutCtor() {
    const a = new A12();
    return a.b;
}
