/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I {
    x: number;
    y: boolean;
}

export function infc16() {
    const i: I = { y: true, x: 10 };
    const b = 20;
    i.x = b;
    return i.x;
}
