/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function arrayTest19(n: number) {
    const array1 = new Array<Array<boolean>>(new Array<boolean>(n));
    return array1;
}
