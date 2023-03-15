/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function objTest() {
    const i = (m: number) => {
        return m * m;
    };
    const obj = {
        y: 11,
        x: i,
        z: {
            k: false,
            j: (x: number, y: number) => {
                return x + y;
            },
        },
    };
    return obj.z.j(8, 9) + obj.x(10);
}
