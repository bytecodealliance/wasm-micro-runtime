/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I {
    x: number;
    z: string;
}

interface I2 {
    y: boolean;
    x: number;
    z: string;
}

interface I3 {
    x: number;
    y: boolean;
}

export function fieldAssignToOther() {
    const i1: I2 = { x: 1, y: true, z: 'str' };
    const i: I = i1;
    const b = i.x;
    return b;
}

export function otherAssignToField() {
    const i: I3 = { y: true, x: 10 };
    const b = 20;
    i.x = b;
    return i.x;
}
