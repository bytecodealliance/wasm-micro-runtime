/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function builtInArray() {
    const a: any = [1, 2, 3];
    const b = Array.isArray(a);
    // console.log(b);
    // const c1 = Array.of();
    // const c2 = Array.of(1, 2);
    // c1[0] = 8
    // console.log(c1);
    // console.log(c2.length)
    // c2[3] = 9
    // console.log(c2);
    // console.log(c2.length)
    // const obj = { a: 1, b: 2, c: 3 };
    // const e = Array.from(Object.keys(obj))
    // const e2 = Array.from([1, 2])
    // const f = Array.toString();
    return b;
}

// builtInArray();
