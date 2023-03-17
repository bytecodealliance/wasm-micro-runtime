/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

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

export function classTest9() {
    let a: A9 = new A9(10);
    let i = a._a;
    let j = a.test(5);
    let k = a.a;
    return i + j + k;
}
