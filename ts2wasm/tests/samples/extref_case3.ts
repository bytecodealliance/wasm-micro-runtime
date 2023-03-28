/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function extrefTest() {
    const str = 'hi';
    const a: any = str;
    return a as string;
}
