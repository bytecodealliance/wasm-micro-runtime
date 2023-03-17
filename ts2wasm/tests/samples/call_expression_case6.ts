/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function callInternalReturnTest(a: number, b = 2) {
    function callReturnTest(a: number, b: number, c: number) {
        return a + b + c;
    }
    const c = callReturnTest(a, b, 3);
    const d = callReturnTest(1, 2, 3);
    return c + d;
}
