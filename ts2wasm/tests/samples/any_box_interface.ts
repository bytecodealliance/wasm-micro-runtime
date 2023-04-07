/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I {
    x: number;
    y: boolean;
}

export function boxInterface() {
    const i: I = { x: 1, y: true };
    const a: any = i;
    const c = (a as I).y;
    return c;
}
