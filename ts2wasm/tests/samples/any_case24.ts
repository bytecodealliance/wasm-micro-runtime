/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function anyTest() {
    // with string
    // const var1 = undefined;
    // const var2 = 1;
    // const var3: any = 2;
    // const var4: any = var2;
    // const var5 = var4 as number;
    // const var6 = 'hi';
    // const var7: any = 'hello';
    // const var8: any = var6;
    // const var9 = var8 as string;
    // const var10 = [1, 2];
    // const var11: any = [3, 4];
    // const var12: any = var10;
    // const var13 = var12 as number[];
    // const var14 = { a: 1 };
    // const var15: any = { b: 2 };
    // const var16: any = var14;
    // const var17 = var16 as typeof var14;
    // return var17.a;

    // without string
    const var1 = undefined;
    const var2 = 1;
    const var3: any = 2;
    const var4: any = var2;
    const var5 = var4 as number;
    // const var6 = 'hi';
    // const var7: any = 'hello';
    // const var8: any = var6;
    // const var9 = var8 as string;
    const var10 = [1, 2];
    const var11: any = [3, 4];
    const var12: any = var10;
    const var13 = var12 as number[];
    const var14 = { a: 1 };
    const var15: any = { b: 2 };
    const var16: any = var14;
    const var17 = var16 as typeof var14;
    return var17.a;
}
