/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function scopeScannerCase4Func4(a: number) {
    let b = 2;
    for (let i = 0; i < a; i++) {
        b++;
    }
    return b;
}
// 102
