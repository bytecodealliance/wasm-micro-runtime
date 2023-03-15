/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

// eslint-disable-next-line @typescript-eslint/no-unused-vars, prefer-const
let globalStatementCase5_1 = 99;

// do while statement
do {
    let doWhileVar = 3;
    --globalStatementCase5_1;
    doWhileVar = 4;
} while (globalStatementCase5_1 > 95);
