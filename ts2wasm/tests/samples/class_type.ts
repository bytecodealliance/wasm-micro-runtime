/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

class A17 {
    test() {
        return 1;
    }
}

export function uniqueType() {
    const a = new A17();
    {
        class A17 {
            //
        }
        const b = a.test();
        if (b > 0) {
            return b;
        }
    }
    return -1;
}
