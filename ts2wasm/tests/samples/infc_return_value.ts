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

function foo(): I {
    const i = { y: true, x: 10, z: 'str' };
    return i;
}

function foo2(): Foo {
    const i: I2 = { y: true, x: 10, z: 'str' };
    return i;
}

export function returnInfc() {
    const infc = foo();
    return infc.y;
}

export function returnClass() {
    const obj = foo2();
    return obj.x;
}
