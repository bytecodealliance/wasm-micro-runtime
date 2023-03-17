/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A10 {
    public _a: number;
    constructor(a: number) {
        this._a = a;
    }
    test(m: number) {
        return m;
    }
    test1() {}
}

class B10 extends A10 {
    public _b: number;
    constructor(a: number, b: number) {
        super(a);
        this._b = b;
    }
    test(m: number) {
        return m + this._b;
    }
}

export function classTest10() {
    let a: A10 = new B10(10, 11);
    let i = a._a;
    let j = a.test(5);
    return i + j;
}
