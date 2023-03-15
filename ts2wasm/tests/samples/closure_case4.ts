/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function closure(x: number, y: boolean) {
    function inner() {
        x = 1;
        y = false;
        return x;
    }
    return inner;
}

export function closureTest() {
    const f1 = closure(10, false);
    return f1();
}
