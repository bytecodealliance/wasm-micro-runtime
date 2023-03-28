/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

function outer() {
    function inner1() {
        return false;
    }
    function inner2() {
        return true;
    }
    return [inner1, inner2];
}

export function arrayClosureTest() {
    const arr = outer();
    const res = arr[1];
    return res(); // true
}
