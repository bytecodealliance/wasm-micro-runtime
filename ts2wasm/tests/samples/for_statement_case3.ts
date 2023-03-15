/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function forTest(): number {
    const c = 100;
    // eslint-disable-next-line no-empty
    for (let k = 10; k > 4; --k) {}
    return c;
}
