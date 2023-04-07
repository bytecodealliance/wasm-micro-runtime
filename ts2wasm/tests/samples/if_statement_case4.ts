/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
class A {
    //
}

export function ifTest(x: number) {
    const res: A | null = null;
    if (!res) {
        if (x) {
            return x;
        }
    }
    return -1;
}
