/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function basicSample() {
    const a: number = 1;
    const b: number = 2;
    const c: number = a + b;
    return c;
}

export function select(v: boolean) {
    if (v) {
        return 1;
    } else return 2;
}
