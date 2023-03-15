/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

interface I {
    x: number;
    y: boolean;
}

function infc12() {
    const i = new Array<I>(2);
    i[0] = { y: true, x: 12 };
}
