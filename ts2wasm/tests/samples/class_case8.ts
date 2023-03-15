/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A8 {
    public _a: number;
    constructor(a: number) {
        this._a = a;
    }
    test(m: number) {
        return m;
    }
    test1() {}
}

class B8 extends A8 {
    public _b: number;
    constructor(a: number, b: number) {
        super(a);
        this._b = b;
    }
    test(m: number) {
        return m + this._b;
    }
}

export function classTest8() {
    let a: A8 = new B8(10, 11);
    return a.test(10);
}
