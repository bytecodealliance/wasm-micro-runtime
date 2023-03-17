/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function functionTest() {
    c = 2;
    const a = c;
    // eslint-disable-next-line no-var
    var c = 3;
    const b = c;
    return a + b;
}
