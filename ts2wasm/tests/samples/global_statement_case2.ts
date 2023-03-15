/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

// eslint-disable-next-line @typescript-eslint/no-unused-vars, prefer-const
let globalStatementCase2_1 = 99;
let globalStatementCase2_2: number;
// if statement
if (globalStatementCase2_1 > 2) {
    const ifVar = 33;
    if (ifVar < globalStatementCase2_1) {
        globalStatementCase2_1 = 20;
    }
    globalStatementCase2_2 = 22;
} else {
    globalStatementCase2_2 = 11;
}
