/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function ifTest() {
    const a = 10;
    const b = 5;
    const c = 1;
    const d = 1;
    // eslint-disable-next-line no-empty
    if (a > 1) {
    }
    return a + b + c + d;
}

// 17
