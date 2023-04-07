/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function prefixUnaryPlusPlus() {
    let a = 1;
    ++a;
    a++;
    return a;
}

export function prefixUnaryMinusMinus() {
    let a = 1;
    --a;
    a--;
    return a;
}

export function prefixUnaryExclamation() {
    const a = 0;
    if (!a) {
        return 1;
    }
    return 0;
}

export function prefixUnaryMinusToLiteralWithBinaryExpr() {
    let a: number;
    a = -1;
    return a;
}

export function prefixUnaryMinusToLiteralWithVarStmt() {
    const a = -1;
    return a;
}

export function prefixUnaryMinusToVarWithBinaryExpr() {
    const a = 1;
    let b: number;
    b = -a;
    return b;
}

export function prefixUnaryMinusToVarWithVarStmt() {
    const a = 1;
    const b = -a;
    return b;
}

export function prefixUnaryPlus() {
    let x = 1;
    const a = +x;
    return a;
}
