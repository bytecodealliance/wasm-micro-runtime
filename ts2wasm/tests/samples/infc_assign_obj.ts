/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I {
    x: number;
    y: boolean;
}

export function objLiteralAndInfc() {
    const i: I = { x: 1, y: false };
    let o = { x: 10, y: true };
    o = i;
    return o.x;
}
