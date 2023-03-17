/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function doTest(): number {
    let o = 9;
    let c = 10;
    do {
        c++;
        if (c > 15) {
            break;
        }
    } while (20 > o++);
    return c;
}
