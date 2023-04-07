/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function length() {
    const a = [1, 2, 3];
    const b = [4];
    const aLen = a.length;
    return aLen;
}

export function isArray() {
    const a: any = [1, 2, 3];
    const b = Array.isArray(a);
    return b;
}
