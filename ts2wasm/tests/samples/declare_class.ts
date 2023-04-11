/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

declare class DeclaredClass {
    grade: number;
    constructor(grade: number);
    sayHello(): void;
    static whoSayHi(name: string): number;
}

export function classDecl() {
    const sayHiFunc = DeclaredClass.whoSayHi('i');
    return sayHiFunc;
}
