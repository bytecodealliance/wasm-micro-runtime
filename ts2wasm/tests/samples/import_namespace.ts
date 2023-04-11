/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import { ns2 as nsOtherName } from './export_namespace';

export function importNamespaceFunc() {
    const ns2Result = nsOtherName.one();
    return ns2Result;
}

export function importNamespaceVar() {
    const ns2_var = nsOtherName.v1;
    return ns2_var;
}

export function importNestedNamespaceFunc() {
    const ns3_func = nsOtherName.ns3.three;
    return ns3_func();
}

export function importNestedNamespaceVar() {
    const ns3_var = nsOtherName.ns3.v2;
    return ns3_var;
}
