/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function funcHosting() {
    const a = foo();
    return a;
}

function foo(a = 10, b = 1, c = 99) {
    return a + b + c;
}
