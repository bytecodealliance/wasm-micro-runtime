/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class cpxCase3Class1 {
    x = 10;
}

export function cpxCase3Func1() {
    const a: cpxCase3Class1 = new cpxCase3Class1();
    return a.x;
}
