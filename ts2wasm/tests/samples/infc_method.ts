/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I {
    x: number;
    y: boolean;
    get _x(): number;
    set _x(x: number);
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

export function infcSetter() {
    const f = new Foo();
    const i: I = f;
    i._x = 10;
    return i._x;
}

interface I2 {
    x: number;
    y: boolean;
    test: () => boolean;
}

export function infcMethod() {
    const f = new Foo();
    const i: I2 = f;
    return i.test();
}

export function infcGetter() {
    const f = new Foo();
    const i: I = f;
    const m = i._x;
    return m;
}
