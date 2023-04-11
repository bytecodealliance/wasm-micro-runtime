/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function add(a: number, b: number): number {
    return a + b;
}

function sub(a: number, b: number): number {
    return a - b;
}

export { sub };

function mul(a: number, b: number): number {
    return a * b;
}

export { mul as renamed_mul };

export default function theDefault(): number {
    return 8;
}
