/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function newLiteralBoolean() {
    let array1: boolean[] = [true, false, true];
    return array1[0];
}
