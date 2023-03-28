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

function foo(i: Foo) {
    return i.x;
}

export function infc11(i: (i: Foo) => number) {
    const f: I2 = { x: 10, y: true, z: 'str' };
    const res = i(f);
    return res;
}
