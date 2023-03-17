/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A {
    static c = 10; //10
    static readonly d = 12 + A.c; //22
}

class B extends A {
    static c = 20; // 20 20
}
export function classTest() {
    return A.c + A.d + B.c + B.d;
}
// 74
