/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function beCalledFunc() {
    return 2;
}

export function exportedFunc() {
    return beCalledFunc();
}
