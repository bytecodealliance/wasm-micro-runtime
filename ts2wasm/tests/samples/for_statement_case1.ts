/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function forTest(): number {
    const c = 100;
    for (let i = 2; i < 5; i++);
    return c;
}
