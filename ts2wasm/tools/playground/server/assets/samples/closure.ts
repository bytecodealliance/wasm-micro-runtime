/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

function foo() {
    let x = 10;
    function inner(i: number) {
        return x + i;
    }
    return inner;
}

export function firstClassFuncTest() {
    let func = foo();
    let y = func(11);
    return y; // 21
}
