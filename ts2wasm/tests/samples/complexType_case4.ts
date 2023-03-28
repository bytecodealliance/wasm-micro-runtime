/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class cpxCase3Class1 {
    foo(i: number, j: number) {
        return i + j;
    }
}

export function cpxCase3Func1() {
    const a: cpxCase3Class1 = new cpxCase3Class1();
    let k = a.foo(1, 2);
    return k;
}
