/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function primitiveTest() {
    const priCase1Var1 = 3;
    const priCase1Var2 = 'hi';
    const priCase1Var3 = true;
    const priCase1Var4 = false;
    const priCase1Var5 = null;
    if (priCase1Var3 && !priCase1Var4) {
        return priCase1Var1;
    }
    return -1;
}
// 3
