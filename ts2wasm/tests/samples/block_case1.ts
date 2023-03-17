/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function blockTest() {
    let a: number;
    let b = 1;
    {
        let c = 9;
        a = 2;
        b = 1;
    }
    return a + b;
}
