/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function forTest(): number {
    let c = 100;
    let j = 0;
    for (; j < 10; ++j) {
        c--;
    }
    return c;
}
