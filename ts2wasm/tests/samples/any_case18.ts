/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function anyTest() {
    let anyCase18g1: any = 1;
    let anyCase18g2: any = 2;
    let anyCase18g3: any = anyCase18g1 + anyCase18g2;
    return anyCase18g3;
}
