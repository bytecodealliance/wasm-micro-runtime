/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function switchCaseCase1(): number {
    // nested
    let c = 100;
    let i = 10;
    let k = 13;
    switch (i) {
        case 11: {
            c = 101;
            break;
        }
        case 10: {
            for (let m = 0; m < 5; m++) {
                switch (m) {
                    case 2:
                        c++;
                        break;
                    case 1:
                        {
                            c = 0;
                        }
                        break;
                    default:
                        break;
                }
            }
            break;
        }
    }
    return c;
}
