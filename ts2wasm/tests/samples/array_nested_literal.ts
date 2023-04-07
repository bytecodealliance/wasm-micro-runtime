/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function nestedLiteral() {
    const array1 = [
        [1, 2],
        [3, 4],
    ];
    return array1[1][0];
}
