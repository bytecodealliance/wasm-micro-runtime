/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function consoleLog() {
    let obj: any = {
        a: 1,
        b: 2,
    };
    console.log(1, true, 123, obj);
}
