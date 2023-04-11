/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import theDefault, {
    add,
    sub as sub1,
    renamed_mul as mul1,
} from './export_func';

import { exportedFunc } from './export_func_invoked';

export function importFuncAdd() {
    const a = add(1, 2);
    return a;
}

export function importFuncSub() {
    const a = sub1(1, 2);
    return a;
}

export function importFuncMul() {
    const a = mul1(1, 2);
    return a;
}

export function importDefaultFunc() {
    return theDefault();
}

export function importFuncInvoked() {
    const importedFunc = exportedFunc;
    return importedFunc();
}
