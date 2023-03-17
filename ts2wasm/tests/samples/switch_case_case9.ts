/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function switchCaseCase9() {
    let i: number = 1;
    switch (i) {
        case 4: {
            i = 2;
            break;
        }
        case 2: {
            i = 3;
            break;
        }
    }
    return i;
}
