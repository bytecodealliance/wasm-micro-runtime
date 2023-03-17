/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export class Array {
    @binaryen
    static isArray(x: any): boolean {
        return Array.isArray(x);
    }
}

function binaryen(target: any, propertyKey: string, descriptor: any) {
    // decorator logic here
}
