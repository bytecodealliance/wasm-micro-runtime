/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/* eslint-disable prefer-const */
/* eslint-disable @typescript-eslint/no-unused-vars */

export function primitiveTest() {
    let priCase2Var1 = 3;
    let priCase2Var2 = 'hi';
    let priCase2Var3 = true;
    let priCase2Var4 = false;
    let priCase2Var5 = null;
    let priCase2Var6 = priCase2Var1;
    const priCase2Var7 = priCase2Var1;

    if (priCase2Var3 && !priCase2Var4) {
        return priCase2Var6 + priCase2Var7;
    }
    return -1;
}
// 6
