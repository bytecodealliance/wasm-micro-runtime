/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function addAnyAndNumber() {
    let a: any = 1;
    let b = 2;
    let c: any = a + b;
    return c + b;
}
