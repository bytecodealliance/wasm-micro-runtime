/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A6 {
    public _a: number;
    public _b: number;

    constructor(a: number, b: number) {
        this._a = a;
        this._b = b;
    }
}

class B6 extends A6 {
    _c: number;
    constructor(a: number, b: number, c: number) {
        super(a, b);
        this._c = c;
    }
}

export function classTest6() {
    let a: A6 = new B6(10, 20, 30);
    let res = a._a + a._b;
    let b = new B6(10, 20, 30);
    res += b._a + b._b + b._c;
    return res;
}
