/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A7 {
    public _a: number;
    constructor(a: number) {
        this._a = a;
    }
    set a(m: number) {
        this._a = m;
    }
    get a() {
        return this._a;
    }
    test(m: number) {
        return 10;
    }
}

class B7 extends A7 {
    constructor(a: number) {
        super(a);
    }
    test(m: number) {
        return m;
    }
    set a(m: number) {
        this._a = m;
    }
    get a() {
        return this._a;
    }
}

export function classTest7() {
    let a: A7 = new B7(10);
    a.a = 20;
    let b = new B7(10);
    b.a = 20;
    return a.a + b.a;
}
