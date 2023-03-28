/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function extrefTest() {
    // with string
    // const obj1 = {
    //     a: 1,
    //     b: true,
    //     c: 'hi',
    // };
    // without string
    const obj1 = {
        a: 1,
        b: true,
        // c: 'hi',
    };
    const objAny: any = obj1;
    const objAny1 = objAny as typeof obj1;
    return objAny1.a;
}
