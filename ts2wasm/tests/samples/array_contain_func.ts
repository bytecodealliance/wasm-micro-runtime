/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/* eslint-disable @typescript-eslint/no-empty-function */

function inner1() {
    return 10;
}
function inner2() {
    return 20;
}
function outer() {
    return [inner1, inner2];
}

export function containFunc() {
    const arr = outer();
    const res = arr[1];
    return res(); // 20
}
