/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export class SyntaxError extends Error {
    constructor(message: string) {
        super(message);
    }
}

export class UnimplementError extends Error {
    constructor(message: string) {
        super(message);
    }
}
