/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

function outer() {
    let i = 10;
    function inner1() {
        i++;
        return i;
    }
    function inner2() {
        i--;
        return i;
    }
    return [inner1, inner2];
}

export function containClosure() {
    const arr = outer();
    const res = arr[0];
    return res(); // 11
}
