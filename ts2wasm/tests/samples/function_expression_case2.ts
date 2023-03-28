/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

const fe_case1_2 = (a: number) => a + 1;

export function functionTest() {
    return fe_case1_2(1);
}
