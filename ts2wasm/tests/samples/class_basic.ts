/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A1 {
    // empty constructor
    test() {
        return 123;
    }

    test2() {
        return 1;
    }
}

class A2 {
    public _a: number;
    constructor(a: number) {
        this._a = a;
    }
    public testFunc() {
        this._a = 10;
    }
    get a() {
        return this._a;
    }
    set a(m: number) {
        this._a = m;
    }
}

export function withoutCtor() {
    let a: A1 = new A1();
    let b = a.test();
    return b;
}

export function basic() {
    let a: A2 = new A2(10);
    return a.a;
}

class A9 {
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
        return m;
    }
    test1() {}
}

export function getterSetter() {
    let a: A9 = new A9(10);
    let i = a._a;
    a.a = i;
    let j = a.test(5);
    let k = a.a;
    return i + j + k;
}
