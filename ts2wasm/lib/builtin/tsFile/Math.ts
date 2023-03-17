/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export class Math {
    static pow(x: number, y: number): number {
        let res = 1;
        let power = y < 0 ? -y : y;
        while (power > 0) {
            res = res * x;
            power--;
        }
        res = y < 0 ? 1 / res : res;
        return res;
    }

    static max(x: number[]): number {
        const arrLen = x.length;
        let res = x[0];
        for (let i = 1; i < arrLen; i++) {
            if (res < x[i]) {
                res = x[i];
            }
        }
        return res;
    }

    static min(x: number[]): number {
        const arrLen = x.length;
        let res = x[0];
        for (let i = 1; i < arrLen; i++) {
            if (res > x[i]) {
                res = x[i];
            }
        }
        return res;
    }

    @binaryen
    static sqrt(x: number): number {
        return Math.sqrt(x);
    }

    @binaryen
    static abs(x: number): number {
        return Math.abs(x);
    }

    @binaryen
    static ceil(x: number): number {
        return Math.ceil(x);
    }

    @binaryen
    static floor(x: number): number {
        return Math.floor(x);
    }

    @binaryen
    static trunc(x: number): number {
        return Math.trunc(x);
    }
}

function binaryen(target: any, propertyKey: string, descriptor: any) {
    // decorator logic here
}
