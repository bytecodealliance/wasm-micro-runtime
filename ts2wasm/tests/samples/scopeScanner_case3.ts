/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function scopeScannerCase3Func3(a: number) {
    let b = 2;
    if (a > b) {
        return 2;
    } else {
        b++;
    }
    return b;
}
// 3
