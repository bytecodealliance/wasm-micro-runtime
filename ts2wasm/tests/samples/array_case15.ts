/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/* eslint-disable @typescript-eslint/no-empty-function */

function inner1() {
    //
}
function inner2() {
    //
}
export function outer() {
    return [inner1, inner2];
}
