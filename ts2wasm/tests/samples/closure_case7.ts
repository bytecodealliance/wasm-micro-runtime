/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

let y = '123';

function ClosureTestCase7() {
    function inner1() {
        return y;
    }
    return inner1;
}
