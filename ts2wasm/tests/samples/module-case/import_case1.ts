/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import { add as add1 } from './export-case1';
import { add as add2 } from '../export-case1';
import { add } from '../export-case1';
import { declare1_add, declare1_a } from './declare-case1';

const import1_a1 = add1(1, 2);
const import1_a2 = add2(3, 4);
const import1_a3 = declare1_add(5, 6);
const import1_a4 = declare1_a;

// const declare1_add2 = declare1_add;
// declare1_add2(7, 8);

function addd(a: number, b: number) {
    return a + b;
}

add(1, 2);

// const add333 = add;
// add333(3, 3);
