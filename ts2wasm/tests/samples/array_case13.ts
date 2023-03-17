/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function outer() {
    let i = 10;
    function inner1() {
        i++;
    }
    function inner2() {
        i--;
    }
    return [inner1, inner2];
}
