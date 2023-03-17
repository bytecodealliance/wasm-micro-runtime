/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export function add(a: number, b: number): number {
    return a + b;
}

function sub(a: number, b: number): number {
    return a - b;
}

export { sub };

function mul(a: number, b: number): number {
    // not exported as "mul"
    return a * b;
}

export { mul as renamed_mul };

export const a = 1;

const b = 2;

export { b };

const c = 3; // not exported as "c"

export { c as renamed_c };

export namespace ns {
    function one(): void {}
    export function two(): void {}
}

export default ns;

export declare namespace ns2 {
    function one(): number;
    const v1: string;
    export function two(): void;

    export namespace ns3 {
        const v2: number;
        function three(): boolean;
    }
}
