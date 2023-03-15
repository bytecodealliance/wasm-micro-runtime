/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function funcvv3() {
    vv3 = 5;
    {
        var vv3 = 9;
    }
    {
        var vv3 = 10;
    }
    function funcvv3_inner() {
        var vv3 = 15;
    }
    return vv3;
}
