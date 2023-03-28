/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A {
    test() {
        return 1;
    }
}

export function cpxCase3Func1() {
    const arr = [new A(), new A(), new A()];
    const arr2 = [arr];
    const yyy = arr2[arr[0].test() - 1][2].test() + 5;
    return yyy; // 6
}
