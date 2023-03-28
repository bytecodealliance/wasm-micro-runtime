/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I {
    x: number;
    y: boolean;
}

interface I2 {
    y: boolean;
    x: number;
    z: string;
}

class Foo {
    y: boolean;
    z: string;
    x: number;
    constructor() {
        this.x = 1;
        this.y = false;
        this.z = 'str';
    }
}

function foo(i: I2) {
    const f: Foo = i;
    return f.x;
}

export function infc8() {
    const i2: I2 = { y: false, x: 100, z: 'z' };
    const res = foo(i2);
    return res;
}
