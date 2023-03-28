/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import theDefault from './export_case1';
import qq from './export_case2';

export function impExpTest() {
    theDefault.two();
    qq();
}
