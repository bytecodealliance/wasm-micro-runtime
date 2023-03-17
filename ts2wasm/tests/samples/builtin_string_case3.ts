/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function strTest(): number {
    const a: string = 'hello';
    const b: number = a.length;
    return b;
}
