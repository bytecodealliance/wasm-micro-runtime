/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function extrefTest() {
    let a: any;
    const num1 = 1;
    a = num1;
    const num2 = a as number;
    return num2;
}
