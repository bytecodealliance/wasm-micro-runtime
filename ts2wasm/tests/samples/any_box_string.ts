/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function boxStringWithVarStmt() {
    let a: any = 'hello';
    return a;
}

export function boxStringWithBinaryExpr() {
    let a: any;
    a = 'hello';
    return a;
}
