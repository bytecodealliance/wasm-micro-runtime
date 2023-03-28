/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function scopeScannerCase8Func8(a: number) {
    let b = 2;
    {
        const innerBlock = 1;
        if (a > b) {
            return 2;
        } else {
            for (let i = 0; i < a; i++) {
                b++;
            }
        }
    }
    return b;
}
