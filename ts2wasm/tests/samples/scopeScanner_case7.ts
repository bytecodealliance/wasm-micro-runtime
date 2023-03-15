/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

function scopeScannerCase7Func1(a: number) {
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
}
