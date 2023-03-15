/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function arrayTest7() {
    const array1 = new Array<number>(1);
    array1[0] = 3;
    return array1;
}
