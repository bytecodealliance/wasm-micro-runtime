/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

const cpxCase1Var1 = [1, 2, 3];
const cpxCase1Var2 = [2, 'hi'];
const cpxCase1Var3 = [[2, 3], ['hi']];
const cpxCase1Var4 = [{ a: 1 }, { a: 2 }, { a: 3, b: 3 }];
const cpxCase1Var5 = [{ a: 1 }, { a: 2 }];
const cpxCase1Var6 = new Array<number>(2);
// class cpxCase1Class1 {}
// const cpxCase1Var7 = [new cpxCase1Class1(), new cpxCase1Class1()];
function cpxCase1Func1(a: number) {
    return 1;
}
const cpxCase1Var8 = [cpxCase1Func1];
function cpxCase1Func2(a: number) {
    return 'hi';
}
const cpxCase1Var9 = [cpxCase1Func1, cpxCase1Func2];
const cpxCase1Var10 = { a1: 2, b: 'hello' };
const cpxCase1Var11 = { a: 2, b2: 'hello' };
