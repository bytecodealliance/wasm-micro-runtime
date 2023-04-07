/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function notNumber(): boolean {
    const j1 = 123;
    const i4 = !j1;
    return i4;
}

export function notBoolean(): boolean {
    const j2 = false;
    const i5 = !j2;
    return i5;
}

export function notWithLogicOperator(): number {
    let i = 10,
        j = 10;
    let k = !(!i && !j);
    return 0;
}
