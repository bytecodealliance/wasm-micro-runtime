/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function getProp() {
    let obj: any = {};
    obj.a = 1;
    obj.length = 4;
    const b = obj.length;
    return b;
}
