/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import exportDefaultFunc from './module_case8'

export default function testFunc() {
    return exportDefaultFunc(10);
}
