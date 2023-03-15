/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function booleanTestCase7(): number {
    let i = 10,
        j = 10;
    let k = i && !j;
    return 0;
}
