/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function castAnyBackToClass() {
    const obj1 = {
        a: 1,
        b: true,
        // c: 'hi',
    };
    const objAny: any = obj1;
    const res = objAny as typeof obj1;
    return res.b;
}

export function castAnyBackToString() {
    const str = 'hi';
    const a: any = str;
    return a as string;
}

export function castAnyBackToNumber() {
    const v = false;
    let a: any = v;
    const num = 1;
    a = num;
    return a as number;
}

export function castAnyBackToNull() {
    const extrefIden = null;
    const a: any = extrefIden;
    return a as null;
}

export function castAnyBackToBoolean() {
    const extrefIden = true;
    const a: any = extrefIden;
    return a as boolean;
}

export function castAnyBackToUndefined() {
    const extrefIden = undefined;
    const a: any = extrefIden;
    return a as undefined;
}
