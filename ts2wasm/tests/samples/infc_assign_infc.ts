/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I {
    x: number;
    y: boolean;
}

interface I2 {
    y: boolean;
    x: number;
    z: string;
}

export function infcAndInfc() {
    const i: I2 = { y: true, x: 10, z: 'str' };
    const f: I = i;
    return f.x;
}
