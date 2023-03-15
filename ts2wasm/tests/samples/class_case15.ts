/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A15 {
    static test() {
        return 1;
    }
    // eslint-disable-next-line @typescript-eslint/no-empty-function
    constructor() {}
}

class B15 extends A15 {
    constructor() {
        super();
    }
    static test() {
        return 2;
    }
}
export function classTest15() {
    return B15.test();
}
