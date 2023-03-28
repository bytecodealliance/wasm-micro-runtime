/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

// eslint-disable-next-line @typescript-eslint/no-unused-vars, prefer-const
let globalStatementCase6_1 = 1;

export function globalTest() {
    switch (globalStatementCase6_1) {
        case 4: {
            globalStatementCase6_1 = 2;
            break;
        }
        case 2: {
            globalStatementCase6_1 = 3;
            break;
        }
        case 1: {
            j = 20;
            globalStatementCase6_1 = j;
            var j = 10;
            break;
        }
    }
    return globalStatementCase6_1;
}
