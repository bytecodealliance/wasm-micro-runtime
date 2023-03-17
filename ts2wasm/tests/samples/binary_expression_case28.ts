/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function binaryExpressionTest() {
    const a = 10,
        b = 20;
    const condition = a || b;
    if (condition) {
        return condition;
    }
    return 0;
}

// 10
