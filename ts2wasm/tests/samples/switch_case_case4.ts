/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function switchCaseCase4(): number {
    // without default
    let i = 10;
    let j = 0;
    switch (i) {
        case 10: {
            j = 10;
            break;
        }
        case 11: {
            j = 11;
            break;
        }
    }
    return j;
}
