/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

namespace NSCase1 {
    export const a = 1;
    export const b = true;
}

export function namespaceTest() {
    const nscase1_global1 = NSCase1.a;
    return nscase1_global1;
}
// 1
