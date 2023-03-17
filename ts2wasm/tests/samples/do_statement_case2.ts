/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function doTest() {
    const o = 9;
    const c = 10;
    // eslint-disable-next-line no-empty
    do {} while (c > 100);
    return c;
}
