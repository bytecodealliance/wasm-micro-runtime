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

function foo(): I {
    const i = { y: true, x: 10, z: 'str' };
    return i;
}

export function infc4() {
    const infc = foo();
    return infc.y;
}
