/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function closure(x: number, y: boolean) {
    let z = 1;
    function inner() {
        function inner1(a: number) {
            let m = 1;
            return m + z;
        }
        return inner1;
    }
    return inner;
}

export function closureTest() {
    const f1 = closure(1, false);
    const f2 = f1();
    const res = f2(1);
    return res;
}
