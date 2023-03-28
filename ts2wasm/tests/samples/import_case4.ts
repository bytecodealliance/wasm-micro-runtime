/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import * as other from './export_case1';

// other.add(other.a, other.b) +
// other.sub(other.b, other.renamed_c) +
// other.renamed_mul(other.renamed_c, other.a);

export function impExpTest() {
    other.ns.two();
}
