/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

const cpxCase2Var1 = [1, 2, 3];
const cpxCase2Var2 = [2, 'hi'];
const cpxCase2Var5 = [{ a: 1 }, { a: 2 }];
const cpxCase2Var6 = new Array<number>(2);
// class cpxCase2Class1 {}
// const cpxCase2Var7 = [new cpxCase2Class1(), new cpxCase2Class1()];
function cpxCase2Func1(a: number) {
    return 1;
}
const cpxCase2Var8 = [cpxCase2Func1];
function cpxCase2Func2(a: number) {
    return 'hi';
}

export function cpxCase2Func3(a: number) {
    let b = 2;
    {
        const cpxCase2Var3 = [[2, 3], ['hi']];
        const cpxCase2Var4 = [{ a: 1 }, { a: 2 }, { a: 3, b: 3 }];
        const innerBlock = 1;
        if (a > b) {
            return 2;
        } else {
            for (let i = 0, j = 'hi'; i < a; i++) {
                b++;
                const cpxCase2Var12 = [[1]];
            }
            const cpxCase2Var9 = [cpxCase2Func1, cpxCase2Func2];
            const cpxCase2Var10 = { a1: 2, b: 'hello' };
            const cpxCase2Var11 = { a: 2, b2: 'hello' };
        }
    }
}
