/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function anyTest1() {
    let a: any = 1;
    let b: any = 2;
    let c: any;
    c = a + b;
    return c; // 3
}

export function anyTest2() {
    let obj: any = { a: 1 };
    let b = (obj.a as number) + 1;
    return b; // 2
}
