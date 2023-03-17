/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function whileTest(): number {
    let c = 100;
    while (c > 90) {
        c = 10;
    }
    return c;
}
