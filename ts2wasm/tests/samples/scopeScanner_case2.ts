/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function scopeScannerCase2Func2(a: number) {
    let b = 2;
    {
        const innerBlock = 1;
        b += innerBlock;
    }
    return b;
}
