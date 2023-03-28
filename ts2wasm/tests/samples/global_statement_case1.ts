/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

let globalStatementCase1_1: number;
// expression statement
// eslint-disable-next-line @typescript-eslint/no-unused-vars, prefer-const
globalStatementCase1_1 = 99;

export function globalTest() {
    return globalStatementCase1_1;
}
