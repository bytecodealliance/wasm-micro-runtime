/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A3 {
    public _a: number;
    public _b: boolean;

    constructor(a: number, b: boolean) {
        this._a = a;
        this._b = b;
        let c = this._b;
    }
}

export function classTest() {
    let a: A3 = new A3(10, true);
    if (a._b) {
        return a._a;
    }
    return a._a - 10;
}
