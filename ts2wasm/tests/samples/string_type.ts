/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function stringNotReturned() {
    const a: string = 'hello';
}

export function returnString(): string {
    const a: string = 'hello';
    return a;
}

export function assignStringToVariable() {
    let a: string;
    a = 'hello';
    return a;
}

export function noExplicitStringKeyword() {
    const a = '';
    return a;
}

export function unicode() {
    const a: string = '🀄';
    return a;
}
