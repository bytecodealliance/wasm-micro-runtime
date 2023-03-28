/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I {
    x: number;
    y: boolean;
}

class Foo {
    y: boolean;
    x: number;
    constructor() {
        this.x = 1;
        this.y = false;
    }
}

export function infc1() {
    const i: I = { x: 1, y: false };
    const f: Foo = i;
    return f.x;
}
