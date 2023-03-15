/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function anyTest() {
    let obj: any = { a: 1 };
    let b = obj.a + 1;
    return b;
}
