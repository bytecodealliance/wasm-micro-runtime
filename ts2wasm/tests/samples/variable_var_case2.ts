/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function varType() {
    vv2 = 3;
    var vv2: number;
    var vv2 = 8;
    const vv2_const = vv2;
    return vv2 + vv2_const;
}
// 16
