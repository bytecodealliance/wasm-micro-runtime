/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function mathTest() {
    const a = Math.max(3);
    const b = Math.max(1, 2, 4, 8, 9);
    const c = Math.min(1, 2, 4, 8, 9);
    const d = Math.min(2);
    const e = Math.pow(3, 0);
    const f = Math.pow(3, -2);
    const g = Math.pow(3, Math.abs(-3));
    return b;
}
