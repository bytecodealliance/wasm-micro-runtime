/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function callNoReturnTest(a: number, b = 2) {
    const c = a + b;
    return c;
}

export function callExpressionTest() {
    return callNoReturnTest(2, 3);
}
