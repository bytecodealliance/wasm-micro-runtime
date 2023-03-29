/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import DefaultExportClass from "./module_case10";

let a = new DefaultExportClass();

class TestClass extends DefaultExportClass {

}

let b = new TestClass();

export function test() {
    return TestClass.foo(1) + b.bar(2);
}
