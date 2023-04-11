/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

namespace NSCaseFunc {
    export function case2() {
        return 2;
    }
    case2();
}

export function namespaceFunc() {
    const nsFunc = NSCaseFunc.case2;
    return nsFunc();
}
