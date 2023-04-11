/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

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
