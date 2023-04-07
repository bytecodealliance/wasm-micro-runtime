/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function boxAny() {
    let a: any = 1;
    let c: any;
    c = a;
    return c;
}
