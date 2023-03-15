/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function whileTest(): number {
    let c = 100;
    while (c > 0) {
        c--;
        for (let i = 0; i < 100; i++) {
            c--;
            if (c < 50) {
                break;
            }
        }
        break;
    }
    return c;
}
