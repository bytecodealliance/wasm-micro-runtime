/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

namespace NSCaseVar {
    export const a = 1;
    export const b = true;
}

export function namespaceVar() {
    const nsA = NSCaseVar.a;
    return nsA;
}
