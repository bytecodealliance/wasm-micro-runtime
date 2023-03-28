/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I {
    x: number;
    y: boolean;
    test: () => boolean;
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
    test() {
        return this.y;
    }

    set _x(x: number) {
        this.x = x;
    }

    get _x() {
        return this.x;
    }
}

export function infc19() {
    const f = new Foo();
    const i: I = f;
    return i.test();
}
