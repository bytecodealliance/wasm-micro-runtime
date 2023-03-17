/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function switchCaseCase8(): number {
    // nested switch-case statements
    let i = 10;
    let j = 11;
    let k = 0;
    switch (i) {
        case 10:
            switch (j) {
                case 10: {
                    j = 10;
                    break;
                }
                case 11: {
                    j = 11;
                    break;
                }
            }
            break;
        case 12:
            j = 12;
            break;
    }
    return j;
}
