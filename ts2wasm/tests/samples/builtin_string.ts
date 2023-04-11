/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function stringConcat() {
    const a: string = 'hello';
    const b: string = a.concat('world');
    return b;
}

export function stringLength() {
    const a: string = 'hello';
    const b: number = a.length;
    return b;
}

export function stringSliceWithTwoNegativeNumber() {
    const a: string = 'hello';
    const b: string = a.slice(-1, -3);
    return b;
}

export function stringSliceWithTwoPositiveNumber() {
    const a: string = 'hello';
    const b: string = a.slice(1, 3);
    return b;
}

export function stringSliceWithTwoUndefind() {
    const a: string = 'hello';
    const b: string = a.slice(undefined, undefined);
    return b;
}
