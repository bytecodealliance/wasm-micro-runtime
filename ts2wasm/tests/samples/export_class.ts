/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export class ExportedClass {
    _a = 8;

    set a(v: number) {
        this._a = v;
    }

    get a(): number {
        return this._a;
    }

    setA(a: number) {
        this._a = a;
    }
}

export default class DefaultExportClass {
    public static foo(x: number) {
        return x + 1;
    }

    public bar(x: number) {
        return x * 2;
    }
}
