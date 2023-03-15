/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function callTest(a: any) {
    function inner(b: number) {
        return b;
    }
    return inner;
}

callTest(1)(2);
