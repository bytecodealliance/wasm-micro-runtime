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

export function extendWithNewProp() {
    let a: A6 = new B6(10, 20, 30);
    let res = a._a + a._b;
    let b = new B6(10, 20, 30);
    res += b._a + b._b + b._c;
    return res;
}

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

export function methodOverwrite() {
    let a: A7 = new B7(10);
    a.a = 20;
    let b = new B7(10);
    b.a = 20;
    return a.a + b.a;
}
