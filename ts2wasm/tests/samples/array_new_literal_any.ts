/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function newLiteralExplicitAny() {
    const array1: any[] = [1, 'hi', true, { a: 1 }];
    return array1[0];
}

export function newLiteralNonExplicitAny() {
    const array1 = [1, 'hi', true, { a: 1 }];
    return array1[0];
}
