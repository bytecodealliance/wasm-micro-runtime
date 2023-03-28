/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function scopeScannerCase7Func7(a: number) {
    let b = 2;
    switch (a) {
        case 1: {
            b++;
            break;
        }
        case 2: {
            b--;
            break;
        }
        default:
            a += 2;
    }
    return a + b;
}
