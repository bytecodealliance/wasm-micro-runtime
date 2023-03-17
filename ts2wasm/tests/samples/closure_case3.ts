/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function closure(x: number, y: boolean) {
    let z = 1;
    z += 10;
    function inner() {
        z = 10;
        return z;
    }
    return inner;
}

export function closureTest() {
    const f1 = closure(1, false);
    return f1();
}
