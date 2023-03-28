/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import {
    add,
    sub as sub,
    renamed_mul as mul,
    a,
    b as b1,
    renamed_c as c,
    ns as renamed_ns,
} from './export_case1';

export function impExpTest() {
    const a = add(c, b1);
    renamed_ns.two();
    return a;
}
