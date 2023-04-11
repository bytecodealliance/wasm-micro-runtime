/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function deadCodeAfterReturn(a: number, b: number) {
    if (a > b) {
        return a - b;
        a += 1;
    }
    return a;
}

export function deadReturnStatement(a: number) {
    if (a > 0) {
        return a;
        return 'hi';
    }
}
