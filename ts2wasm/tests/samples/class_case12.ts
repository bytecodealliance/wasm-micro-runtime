/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A12 {
    a = 10;
    b = false;
    c = 'c';
}

export function classTest12() {
    const a = new A12();
    return a.b;
}
