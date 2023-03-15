/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function booleanTestCase5(): boolean {
    const j5 = 123,
        j6 = 456;
    const i7 = j5 > 200 || j6 > 200;
    return i7;
}
