/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import { ExportedClass } from './export_class';

export function importClass() {
    const ec = new ExportedClass();
    ec.a = 10;
    return ec.a;
}

import DefaultExportClass from './export_class';

let defaultExportClass = new DefaultExportClass();

class TestClass extends DefaultExportClass {
    public bar(x: number) {
        return x * 1;
    }
}

let testClass = new TestClass();

export function test() {
    return TestClass.foo(1) + testClass.bar(2) + defaultExportClass.bar(2);
}
