/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function boxEmptyObj() {
    let a: any = {};
    return a;
}

export function boxObjWithNumberProp() {
    let obj: any = {
        a: 1,
    };
    return obj.a;
}

export function boxObjWithBooleanProp() {
    let obj: any;
    obj = {
        c: true,
    };
    return obj.c;
}

export function boxNestedObj() {
    let obj: any;
    obj = {
        a: 1,
        c: true,
        d: {
            e: 1,
        },
    };
    return obj.d.e as number;
}
