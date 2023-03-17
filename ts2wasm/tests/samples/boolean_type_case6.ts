/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function booleanTestCase6(): boolean {
    const j7 = 123,
        j8 = 456;
    const i8 = j7 > 200 && j8 < 200;
    return i8;
}
