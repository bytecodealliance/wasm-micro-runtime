/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

declare class declareClass1 {
    grade: number;
    constructor(grade: number);
    sayHello(): void;
    static whoSayHi(name: string): number;
}

export function classDecl() {
    const sayHiFunc = declareClass1.whoSayHi('i');
    return sayHiFunc;
}
