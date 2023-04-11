/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function simpleFunctionOnlyReturn() {
    return 1;
}

export function basicFunction(a: number, b: number) {
    return a + b;
}

function functionWithDefaultParameter(a = 1, b = 2) {
    return a + b;
}

export function defaultParamExport() {
    return functionWithDefaultParameter();
}

export function functionWithFuncScopeVariable() {
    c = 2;
    const a = c;
    // eslint-disable-next-line no-var
    var c = 3;
    const b = c;
    return a + b;
}

export function miltipleVariablesInOneStatement() {
    c = 2;
    const a = c,
        b = a;
    // eslint-disable-next-line no-var
    var c = 6;
    let d: number;
    d = 3;
    return a + b + d;
}

