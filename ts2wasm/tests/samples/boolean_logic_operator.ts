/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function logicOr(): boolean {
    const j5 = 123,
        j6 = 456;
    const i7 = j5 > 200 || j6 > 200;
    return i7;
}

export function logicAnd(): boolean {
    const j7 = 123,
        j8 = 456;
    const i8 = j7 > 200 && j8 < 200;
    return i8;
}

export function conditionExpr(): number {
    const j11 = 123,
        j12 = 456;
    let i10 = j11 > j12 ? 1 : 2; // not support const type, for its union type.
    return i10;
}
