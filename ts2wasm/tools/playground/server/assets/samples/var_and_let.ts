/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function var_and_let() {
    let vlet = 5;
    vv = 5;
    {
        let vlet = 12;
        var vv = 9;
    }
    {
        var vv = 10;
    }

    function funcvv3_inner() {
        var vv3 = 18;
    }

    return vv + vlet; // 15
}
