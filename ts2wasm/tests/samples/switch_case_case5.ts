/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function switchCaseCase5(): number {
    // mutiple cases
    let i = 10;
    let j = 0;
    switch (i) {
        case 10:
        case 11: {
            j = 11;
            break;
        }
        default: {
            j = 12;
            break;
        }
    }
    return j;
}
