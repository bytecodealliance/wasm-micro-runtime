/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function closure(x: number, y: boolean) {
    let z = 1;
    function inner() {
        z++;
        function inner1() {
            let b = 2;
            function inner2() {
                b -= 10;
                let c = 'xyz';
                function inner3() {
                    c = '123';
                }
                return inner3;
            }
            return inner2;
        }
        return inner1;
    }
    z++;
    return inner;
}
