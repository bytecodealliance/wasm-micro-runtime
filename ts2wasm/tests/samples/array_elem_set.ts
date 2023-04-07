/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function setElem() {
    const array1: number[] = [1, 2, 3];
    array1[0] = 5;
    return array1[0];
}
