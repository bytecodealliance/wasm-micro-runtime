/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function arrayTest16() {
    const array1 = [new Array<string>('hi')];
    array1[0][0] = 'hello';
    return array1;
}
