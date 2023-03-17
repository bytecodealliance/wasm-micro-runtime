/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function binaryExpressionTest() {
    const a = 0;
    if (!a) {
        return 1;
    }
    return 0;
}
