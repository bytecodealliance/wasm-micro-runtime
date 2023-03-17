/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function protoTest() {
    let prototypeObj: any = {
        height: 1,
    };
    let obj: any = { weight: 2 };
    obj.__proto__ = prototypeObj;
    return obj.height;
}
