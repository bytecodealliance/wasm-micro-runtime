/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export class lib {
    @binaryen
    static string_slice(s: string, x: number, y: number): string {
        return s.slice(x, y);
    }

    @binaryen
    static string_concat(x: string, y: string): string {
        return x.concat(y);
    }
}

function binaryen(target: any, propertyKey: string, descriptor: any) {
    // decorator logic here
}
