/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

const fe_case1_1 = function (a: number, b: number) {
    return a + b;
};

export function functionTest() {
    return fe_case1_1(7.1, 1997);
}
