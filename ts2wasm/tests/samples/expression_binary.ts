/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function lt() {
    const a = 5;
    if (a < 10) {
        return 1;
    }
    return 0;
}

export function gt() {
    const a = 5;
    if (a > 4) {
        return 1;
    }
    return 0;
}

export function le() {
    const a = 5;
    const b = 5;
    if (a <= 5) {
        if (b <= 6) {
            return 1;
        }
    }
    return 0;
}

export function ge() {
    const a = 5;
    const b = 5;
    if (a >= 5) {
        if (b >= 4) {
            return 1;
        }
    }
    return 0;
}

export function eq() {
    const a = 5;
    if (a == 5) {
        return 1;
    }
    return 0;
}

export function seenAsEq() {
    const a = 5;
    if (a === 5) {
        return 1;
    }
    return 0;
}

export function ne() {
    const a = 5;
    if (a != 5) {
        return 0;
    }
    return 1;
}

export function seenAsNe() {
    const a = 5;
    if (a !== 5) {
        return 0;
    }
    return 1;
}

export function add() {
    return 1 + 2;
}

export function sub() {
    return 2 - 1;
}

export function mul() {
    return 2 * 2;
}

export function div() {
    return 2 / 2;
}

export function subEq() {
    let a = 3;
    a -= 2;
    return a;
}

export function addEq() {
    let a = 1;
    a += 2;
    return a;
}

export function mulEq() {
    let a = 3;
    a *= 2;
    return a;
}

export function divEq() {
    let a = 4;
    a /= 2;
    return a;
}
