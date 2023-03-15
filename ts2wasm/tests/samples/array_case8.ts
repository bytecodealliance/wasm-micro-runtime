/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function arrayTest8() {
    const array1 = new Array<string>('hello', 'world');
    array1[0] = 'hi';
    return array1;
}
