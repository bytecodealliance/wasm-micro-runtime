/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function strTest(): string {
    const a: string = 'hello';
    const b: string = a.concat('world');
    return b;
}
