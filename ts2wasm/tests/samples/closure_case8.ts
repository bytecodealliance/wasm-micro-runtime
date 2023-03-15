/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

// as variable
export function FirstClassFuncClosureCase1() {
    let x = 10;
    function inner(i: number) {
        return x + i;
    }
    return inner;
}

export function firstClassFuncTest() {
    let inner = FirstClassFuncClosureCase1();
    let y = inner(11);
    return y;
}
