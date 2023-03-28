/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I {
    x: number;
    y: boolean;
    z: string;
}

interface I2 {
    y: boolean;
    x: number;
}

export function anyTest() {
    const i: I = { x: 1, z: 'str', y: true };
    const a: any = i;
    const b = a as I;
    const c: I2 = b;

    return c;
}
