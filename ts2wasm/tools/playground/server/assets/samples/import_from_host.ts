/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

declare function host_func(): number;

export function testFunc() {
    /* As closure */
    let f = host_func;

    /* Direct call */
    return host_func() + f();
}
