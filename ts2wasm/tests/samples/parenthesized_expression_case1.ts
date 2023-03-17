/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function parenthesizedTest() {
    const a = 1;
    const b = 6;
    const c = b - a;
    const d = ((a + b) * c) / b;
    return d;
}
