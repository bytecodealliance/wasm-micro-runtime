/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

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

export function classTest() {
    let a: A2 = new A2(10);
    return a.a;
}
