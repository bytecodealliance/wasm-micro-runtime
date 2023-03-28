/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

const globalVariable_case2_1 = 99;

export function globalVarTest() {
    // eslint-disable-next-line @typescript-eslint/no-unused-vars, prefer-const
    let globalVariable_case2_2: number;
    globalVariable_case2_2 = globalVariable_case2_1;
    return globalVariable_case2_1 == globalVariable_case2_2;
}
