/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function anyTest() {
    let obj: any;
    obj = {
        a: 1,
        // b: 'hi',
        c: true,
        d: {
            e: 1,
        },
    };
    return obj.d.e as number;
}
