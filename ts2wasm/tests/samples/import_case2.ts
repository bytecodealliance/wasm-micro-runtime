/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import { ns2 as nsOtherName } from './export_case1';

export function impExpTest() {
    const ns2Result = nsOtherName.one();
    const ns2_var = nsOtherName.v1;
    const ns3_var = nsOtherName.ns3.v2;
    const ns3_func = nsOtherName.ns3.three;
    return ns2Result + ns3_var;
}
