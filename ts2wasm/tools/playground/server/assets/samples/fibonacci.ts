/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function fibonacci(n: number): number {
    if (n == 1) {
        return 1;
    }
    if (n == 2) {
        return 1;
    }
    return fibonacci(n - 2) + fibonacci(n - 1);
}
