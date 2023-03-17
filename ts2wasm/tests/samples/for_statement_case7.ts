/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function forTest(): number {
    let i: number;
    let c: number = 0;

    for (i = 10; i < 100; i++) {
        c += i;
    }

    return c;
}
