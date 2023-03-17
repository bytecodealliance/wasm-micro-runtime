/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function binaryExpressionTest() {
    const a = 1;
    let b: number;
    // eslint-disable-next-line prefer-const
    b = -a;
    return b;
}
