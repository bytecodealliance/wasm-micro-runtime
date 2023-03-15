/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

namespace NSCase2 {
    export function case2() {
        return 2;
    }
    case2();
}

const ns2func1 = NSCase2.case2;
ns2func1();
