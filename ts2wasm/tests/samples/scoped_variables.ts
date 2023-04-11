/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/* eslint-disable @typescript-eslint/no-empty-function */
export function nestedScopes() {
    let x = 1;
    {
        let x = 3;
        do {
            let x = 5;
            return x;
        } while (true);
    }
}
