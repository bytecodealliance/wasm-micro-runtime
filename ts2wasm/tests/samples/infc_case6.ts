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
function testInfc(f: Foo) {
    return f;
}

function infc6() {
    const i: I2 = { x: 1, y: true, z: 'str' };
    const f = testInfc(i);
}
