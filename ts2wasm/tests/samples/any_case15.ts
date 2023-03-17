/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function anyTest() {
    let obj: any = {};
    obj.a = 1;
    obj.b = 'hi';
    obj.length = 4;
    return obj;
}
