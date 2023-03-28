/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I1 {
    x: number;
    z: string;
}

interface I2 {
    y: boolean;
    x: number;
    z: string;
}

export function infc15() {
    const i1: I2 = { x: 1, y: true, z: 'str' };
    const i: I1 = i1;
    const b = i.x;
    return b;
}
