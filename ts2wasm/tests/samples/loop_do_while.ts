/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function loopBodyEmpty() {
    const o = 9;
    const c = 10;
    // eslint-disable-next-line no-empty
    do {} while (c > 100);
    return c;
}

export function basicDoLoop() {
    let c = 10;

    do {
        c++;
    } while (c < 100);
    return c;
}

export function prefixPlusPlus(): number {
    let o = 9;
    let c = 10;
    do {
        c++;
    } while (++o < 20);
    return c;
}

export function suffixPlusPlus(): number {
    let o = 9;
    let c = 10;
    do {
        c++;
    } while (o++ < 20);
    return c;
}

export function numberAsCondition(): number {
    let o = 9;
    let c = 10;

    do {
        c--;
    } while (c);

    return c;
}
