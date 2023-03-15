/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import { add } from './export_case1';

add(1, 1);

export function print2() {
    return 2;
}
