/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function boxNumberWithVarStmt() {
    let a: any = 1;
    return a;
}

export function boxNumberWithBinaryExpr() {
    let a: any;
    a = 1;
    return a;
}
