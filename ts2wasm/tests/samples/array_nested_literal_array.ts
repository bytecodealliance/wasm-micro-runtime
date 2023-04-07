/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function nestedLiteralArrayInOneLayer() {
    const array1 = [new Array<string>('hi')];
    array1[0][0] = 'hello';
    return array1;
}

export function nestedLiteralArrayInMulLayer() {
    const array1 = [new Array<Array<string>>(new Array<string>('hi'))];
    array1[0][0][0] = 'hello';
    return array1;
}
