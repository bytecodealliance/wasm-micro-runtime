/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A {
    test() {
        return 1;
    }
}

let arr = [new A(), new A(), new A()];
let arr2 = [arr];
let yyy = arr2[arr[0].test()][3].test() + 5;
