/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A13 {
    static test1() {
        return 1;
    }

    static test2() {
        return 2;
    }
    // eslint-disable-next-line @typescript-eslint/no-empty-function
    constructor() {}
}

export function classTest13() {
    return A13.test1();
}
