/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function whileTest(): number {
    const c = 100;
    // eslint-disable-next-line no-empty
    while (c > 100) {}
    return c;
}
