/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function selectAmpersandTrueTrue() {
    const a = 10,
        b = 20;
    const condition = a && b;
    if (condition) {
        return condition;
    }
    return 0;
}

export function selectAmpersandTrueFalse() {
    const a = 10,
        b = 20;
    const condition = a && !b;
    if (condition) {
        return 1;
    }
    return 0;
}

export function selectAmpersandFalseTrue() {
    const a = 10,
        b = 20;
    const condition = !a && b;
    if (condition) {
        return 1;
    }
    return 0;
}

export function selectAmpersandFalseFlase() {
    const a = 10,
        b = 20;
    const condition = !a && !b;
    if (condition) {
        return 1;
    }
    return 0;
}

export function selectBarTrueTrue() {
    const a = 10,
        b = 20;
    const condition = a || b;
    if (condition) {
        return condition;
    }
    return 0;
}

export function selectBarTrueFalse() {
    const a = 10,
        b = 20;
    const condition = a || !b;
    if (condition) {
        return 1;
    }
    return 0;
}

export function selectBarFalseTrue() {
    const a = 10,
        b = 20;
    const condition = !a || b;
    if (condition) {
        return 1;
    }
    return 0;
}

export function selectBarFalseFalse() {
    const a = 10,
        b = 20;
    const condition = !a || !b;
    if (condition) {
        return 1;
    }
    return 0;
}
