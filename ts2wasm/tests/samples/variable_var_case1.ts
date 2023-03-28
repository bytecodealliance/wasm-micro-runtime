/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function varType() {
    vp_i1 = 2;
    let vp_1 = vp_i1;
    // eslint-disable-next-line no-var
    var vp_i1 = 1;
    vp_1 = vp_i1;
    return vp_1;
}
// 1
