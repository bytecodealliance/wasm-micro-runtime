/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

// eslint-disable-next-line @typescript-eslint/no-unused-vars, prefer-const
let globalStatementCase3_1 = 99;

export function globalTest() {
    for (let i = 1; i < 5; ++i) {
        i++;
        globalStatementCase3_1++;
    }
    return globalStatementCase3_1;
}
