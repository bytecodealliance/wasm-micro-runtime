/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

// in global scope

export class A16 {
    static hi() {
        return 1;
    }
}

export function classTest16() {
    return A16.hi();
}
