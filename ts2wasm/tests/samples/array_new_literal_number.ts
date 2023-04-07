/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function newLiteralNumberWithLiteralType() {
    const array1: number[] = [1, 2, 3];
    return array1[2];
}

export function newLiteralNumberWithArrayType() {
    const array1: Array<number> = [1, 2, 3];
    return array1[2];
}

export function newLiteralNumberWithoutInit() {
    const array1: Array<number> = [];
    return array1;
}
