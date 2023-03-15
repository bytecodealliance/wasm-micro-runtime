/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I {
    x: number;
    test: () => string;
}

class C1 {
    x: number;
    y: string;
    test() {
        return this.y;
    }
    constructor() {
        this.x = 10;
        this.y = '123';
    }
}

class C2 {
    y: boolean;
    x: number;
    test() {
        return 'test';
    }
    constructor() {
        this.x = 10;
        this.y = false;
    }
}
export function restParameterTest() {
    function bar1(...b: I[]) {
        return b[0];
    }
    const c1 = new C1();
    const c2 = new C2();

    return bar1(c1, c2);
}
