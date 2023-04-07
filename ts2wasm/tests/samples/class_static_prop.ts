/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A15 {
    static test() {
        return 1;
    }
    // eslint-disable-next-line @typescript-eslint/no-empty-function
    constructor() {}
}

class B15 extends A15 {
    constructor() {
        super();
    }
    static test() {
        return 2;
    }
}
export function staticMethodWithOverwrite() {
    return B15.test();
}

export class A16 {
    static hi() {
        return 1;
    }
}

export function staticMethod() {
    return A16.hi();
}

class A {
    static c = 10; //10
    static readonly d = 12 + A.c; //22
}

class B extends A {
    static c = 20; // 20 20
}
export function staticFields() {
    return A.c + A.d + B.c + B.d;
}
