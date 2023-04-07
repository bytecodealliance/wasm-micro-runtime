/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

function closure1(x: number, y: boolean) {
    let z = 1;
    function inner() {
        function inner1(a: number) {
            let m = 1;
            return m + z;
        }
        return inner1;
    }
    return inner;
}

export function accessOuterVars() {
    const f1 = closure1(1, false);
    const f2 = f1();
    const res = f2(1);
    return res;
}

function closure2(x: number, y: boolean) {
    let z = 1;
    z += 10;
    function inner() {
        z = 10;
        return z;
    }
    return inner;
}

export function returnOuterFuncCall() {
    const f1 = closure2(1, false);
    return f1();
}

let y = '123';

export function accesssGlobalVar() {
    function inner1() {
        return y;
    }
    return inner1;
}
