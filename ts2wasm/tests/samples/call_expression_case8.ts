/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

function outer(a: any) {
    function inner(b: number) {
        return b;
    }
    return inner;
}

export function callTest() {
    const res = outer(1)(2);
    return res;
}
