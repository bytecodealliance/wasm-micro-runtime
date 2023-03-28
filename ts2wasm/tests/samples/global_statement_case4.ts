/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

// eslint-disable-next-line @typescript-eslint/no-unused-vars, prefer-const
let globalStatementCase4_1 = 99;
// while statement
export function globalTest() {
    while (globalStatementCase4_1 > 98) {
        let globalStatementCase4_2 = 2;
        globalStatementCase4_2--;
        globalStatementCase4_1--;
    }
    return globalStatementCase4_1;
}
