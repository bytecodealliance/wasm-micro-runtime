/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function useBeforeDeclare() {
    vp_i1 = 2;
    let vp_1 = vp_i1;
    // eslint-disable-next-line no-var
    var vp_i1 = 1;
    vp_1 = vp_i1;
    return vp_1;
}
// 1

export function operateWithConst() {
    vv2 = 3;
    var vv2: number;
    var vv2 = 8;
    const vv2_const = vv2;
    return vv2 + vv2_const;
}
// 16

export function nestedFunction() {
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
// 10
