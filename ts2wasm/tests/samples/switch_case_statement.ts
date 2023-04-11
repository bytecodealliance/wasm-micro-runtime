/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function switchWithDefault(): number {
    let i = 10;
    let j = 0;
    switch (i) {
        case 1: {
            j = 10;
            break;
        }
        case 11: {
            j = 11;
            break;
        }
        default: {
            j = 0;
            break;
        }
    }
    return j;
}

export function nestedSwitchCase(): number {
    let c = 100;
    let i = 10;
    let k = 13;
    switch (i) {
        case 11: {
            c = 101;
            break;
        }
        case 10: {
            for (let m = 0; m < 5; m++) {
                switch (m) {
                    case 2:
                        c++;
                        break;
                    case 1:
                        {
                            c = 0;
                        }
                        break;
                    default:
                        break;
                }
            }
            break;
        }
    }
    return c;
}

export function emptySwitch() {
    const i = 0;
    switch (i) {
    }
    return i;
}

export function switchWithoutDefault(): number {
    let i = 10;
    let j = 0;
    switch (i) {
        case 10: {
            j = 10;
            break;
        }
        case 11: {
            j = 11;
            break;
        }
    }
    return j;
}

export function multipleCasesShareSameBlock(): number {
    let i = 10;
    let j = 0;
    switch (i) {
        case 10:
        case 11: {
            j = 11;
            break;
        }
        default: {
            j = 12;
            break;
        }
    }
    return j;
}

export function caseWithoutBlock(): number {
    let i = 10;
    let j = 0;
    switch (i) {
        case 11:
            j = 11;
            break;
        case 10:
            j = 10;
            break;
    }
    return j;
}

export function caseWithoutBreak(): number {
    let i = 10;
    let j = 0;
    switch (i) {
        case 10:
            j = 10;
        case 11:
            j = 11;
            break;
    }
    return j;
}

export function varDeclarationInCase() {
    let i: number = 1;
    switch (i) {
        case 4: {
            i = 2;
            break;
        }
        case 2: {
            i = 3;
            break;
        }
        case 1: {
            j = 20;
            i = j;
            var j = 10;
            break;
        }
    }
    return i;
}
