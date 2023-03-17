/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function switchCaseCase7(): number {
    // case witchout break
    let i = 10;
    let j = 0;
    switch (i) {
        case 10:
            j = 10;
        case 11:
            j = 11;
            break;
    }
    return j;
}
