/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export default class DefaultExportClass {
    public static foo(x: number) {
        return x + 1;
    }

    public bar(x: number) {
        return x * 2;
    }
}
