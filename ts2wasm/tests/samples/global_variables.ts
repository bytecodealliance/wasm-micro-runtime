/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

const globalNumber = 99;
const globalObject = {
    a: 1,
    b: true
}
const globalAny: any = 100;

export function globalVarTest1() {
    let globalVariable_case1_2: number;
    // eslint-disable-next-line @typescript-eslint/no-unused-vars, prefer-const
    globalVariable_case1_2 = globalNumber;
    return globalNumber + globalVariable_case1_2 + globalObject.a + globalAny;
}

export function globalVarTest2() {
    // eslint-disable-next-line @typescript-eslint/no-unused-vars, prefer-const
    let globalVariable_case2_2: number;
    globalVariable_case2_2 = globalNumber;
    return globalNumber == globalVariable_case2_2;
}

const globalVar1 = 1;
const globalVar2 = 2 + 3;
const globalVar3 = true;
const globalVar4 = 'hello';
const globalVar5: any = 1;
let globalVar6 = 6;
let globalVar7 = [1, 2];
let globalVar8: any = [1, 2];

export function globalVarTest3() {
    return globalVar1 + globalVar2 + globalVar6 + globalVar7[1];
}
// 14
