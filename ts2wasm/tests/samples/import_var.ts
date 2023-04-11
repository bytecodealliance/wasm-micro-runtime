/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import { a, b as b1, renamed_c as c1 } from './export_var';

export function importVarA() {
    return a;
}

export function importVarB() {
    return b1;
}

export function importVarC() {
    return c1;
}
