/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function mathTest() {
    const any1: any = 4;
    const any2: any = 2;
    const e2 = Math.pow(any1 as number, any2 as number);
    return e2;
}
