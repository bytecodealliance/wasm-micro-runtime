/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function simpleObject() {
    const obj1 = {
        a: 1,
        b: true,
        c: 'hi',
    };
    return obj1.a;
}
// 1

export function nestedObject() {
    const obj1 = {
        a: 1,
        b: true,
        c: {
            d: 4,
        },
    };
    return obj1.c.d;
}
// 4

export function moreNestedObject() {
    const obj1 = {
        a: 1,
        b: true,
        c: {
            d: 4,
            e: {
                f: false,
            },
        },
    };
    return obj1.c.e.f;
}

export function assignObjectLiteralToField() {
    const obj1 = {
        a: 1,
        b: true,
        c: {
            d: 4,
        },
    };
    obj1.c = {
        d: 6,
    };
    return obj1.c.d;
}
// 6

export function withMethodField() {
    const i = (m: number) => {
        return m * m;
    };
    const obj = {
        y: 11,
        x: i,
        z: {
            k: false,
            j: (x: number, y: number) => {
                return x + y;
            },
        },
    };
    return obj.z.j(8, 9) + obj.x(10);
}
// 117
