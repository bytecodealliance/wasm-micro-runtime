/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function basicCase(): number {
    let c = 100;
    for (let q = 10; q > 4; --q) {
        c = c + 2;
        c--;
    }
    return c;
}

export function loopBodySemicolon(): number {
    const c = 100;
    for (let i = 2; i < 5; i++);
    return c;
}

export function loopBodyEmpty(): number {
    const c = 100;
    // eslint-disable-next-line no-empty
    for (let k = 10; k > 4; --k) {}
    return c;
}

export function noInitializer(): number {
    let c = 100;
    let j = 0;
    for (; j < 10; ++j) {
        c--;
    }
    return c;
}

export function noCondition(): number {
    let c = 100;
    let j = 0;
    for (j = 0; ; ++j) {
        c--;
        if (j > 10) {
            break;
        }
    }
    return c;
}

export function noIncrement(): number {
    let c = 100;
    let j = 0;
    for (j = 0; j < 10;) {
        c--;
        j++;
    }
    return c;
}

export function nestedForLoopWithBreak(): number {
    let c = 100;
    for (let i = 1; i < 10; i++) {
        c++;
        for (let j = 1; j < 5; j++) {
            c++;
            if (c > 108) {
                break;
            }
        }
        break;
    }
    return c;
}

export function multipleForLoop(): number {
    let c = 100;
    for (let q = 10; q > 4; --q) {
        c = c + 2;
    }
    for (let i = 0; i < 3; i++) {
        c = c + 1;
    }
    return c;
}
