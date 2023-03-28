/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

const globalVariable_case1_1 = 99;

export function globalVarTest() {
    let globalVariable_case1_2: number;
    // eslint-disable-next-line @typescript-eslint/no-unused-vars, prefer-const
    globalVariable_case1_2 = globalVariable_case1_1;
    return globalVariable_case1_1 + globalVariable_case1_2;
}
