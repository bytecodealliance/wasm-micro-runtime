/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A4 {
    constructor() {}
    public test() {
        return 10;
    }
}

export function classTest() {
    const a = new A4();
    return a.test();
}
