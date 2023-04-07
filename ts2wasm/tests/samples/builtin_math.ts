/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function mathSqrt() {
    const a = 9;
    const b = Math.sqrt(a);
    return b;
}

export function mathMaxWithOneOperation() {
    const a = Math.max(3);
    return a;
}

export function mathMaxWithMultiOperation() {
    const a = Math.max(1, 2, 4, 8, 9);
    return a;
}

export function mathMinWithOneOperation() {
    const a = Math.min(2);
    return a;
}

export function mathMinWithMultiOperation() {
    const a = Math.min(1, 2, 4, 8, 9);
    return a;
}

export function mathPowWithZero() {
    const a = Math.pow(3, 0);
    return a;
}

export function mathPowWithNegative() {
    const a = Math.pow(3, -2);
    return a;
}

export function mathPowWithPositive() {
    const a = Math.pow(3, 2);
    return a;
}

export function mathAbs() {
    const a = Math.abs(-3);
    return a;
}

export function mathNested() {
    const a = Math.pow(3, Math.abs(-3));
    return a;
}
