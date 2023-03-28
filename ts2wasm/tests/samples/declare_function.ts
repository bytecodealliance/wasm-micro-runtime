/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

declare function no_param_no_return(): void;
declare function no_param_with_return(): number;
declare function with_param_no_return(x: number): void;
declare function with_param_and_return(x: number): number;

/* Assigned to global variable */
let f1 = no_param_no_return;
f1();
let f2 = no_param_with_return;
let res = f2();
let f3 = with_param_no_return;
f3(5);
let f4 = with_param_and_return;
res = f4(10);

/* Assigned to local variables */
export function testFunc() {
    let f1 = no_param_no_return;
    f1();
    let f2 = no_param_with_return;
    let res = f2();
    let f3 = with_param_no_return;
    f3(5);
    let f4 = with_param_and_return;
    res = f4(10);
}

/* Direct call */
no_param_no_return();
res = no_param_with_return();
with_param_no_return(5);
res = with_param_and_return(10);
