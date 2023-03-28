/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
export function returnTest2(a: number, b: number) {
    if (a > b) {
        return a - b;
        a += 1;
    }
    return a;
}
