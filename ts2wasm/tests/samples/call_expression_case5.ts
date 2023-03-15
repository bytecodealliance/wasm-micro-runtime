/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function callReturnTest(a: number, b: number, c: number) {
    return a + b + c;
}

export function callExpressionTest() {
    return callReturnTest(1, 2, 3);
}
