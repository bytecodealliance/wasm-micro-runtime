/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class Base {
    public _a: number;

    constructor(a: number) {
        this._a = a;
    }

    test(m: number) {
        return m;
    }
    test1() {}
}

class Derived extends Base {
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
    let a: Base = new Derived(10, 11);
    /* Should call Derived's test method */
    return a.test(5); // 16
}
