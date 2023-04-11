/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

declare function noParamNoReturn(): void;
declare function noParamWithReturn(): number;
declare function withParamNoReturn(x: number): void;
declare function withParamWithReturn(x: number): number;

/* Assigned to global variable */
let f1 = noParamNoReturn;
f1();
let f2 = noParamWithReturn;
let res = f2();
let f3 = withParamNoReturn;
f3(5);
let f4 = withParamWithReturn;
res = f4(10);

/* Assigned to local variables */
export function assignDeclareFuncToVar() {
    let f1 = noParamNoReturn;
    f1();
    let f2 = noParamWithReturn;
    let res = f2();
    let f3 = withParamNoReturn;
    f3(5);
    let f4 = withParamWithReturn;
    res = f4(10);
}

/* Direct call */
noParamNoReturn();
res = noParamWithReturn();
withParamNoReturn(5);
res = withParamWithReturn(10);
