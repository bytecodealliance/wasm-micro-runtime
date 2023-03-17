/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function doTest(): number {
    let c = 10;

    do {
        if (c > 20) {
            break;
        }
        c++;
    } while (c < 30);

    return c;
}
