/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function anyTest() {
    let a: any = 1;
    let c: any;
    c = 2 + a + 6;
    return c;
}
