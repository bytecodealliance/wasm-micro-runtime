/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function innerBlock() {
    let a: number;
    let b = 1;
    {
        const c = 9;
        a = 2;
        b = c;
    }
    return a + b;
}
