/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export declare class console {
    static log(...values: any[]): void;
}

export class Array {
    @binaryen
    static isArray(x: any): boolean {
        return false;
    }
}

export class String {
    @binaryen
    concat(...strings: string[]): string {
        return 'concat';
    }

    @binaryen
    slice(start?: number, end?: number): string {
        return 'slice';
    }
}

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
        return 0;
    }

    @binaryen
    static abs(x: number): number {
        return 0;
    }

    @binaryen
    static ceil(x: number): number {
        return 0;
    }

    @binaryen
    static floor(x: number): number {
        return 0;
    }

    @binaryen
    static trunc(x: number): number {
        return 0;
    }
}

function binaryen(target: any, propertyKey: string, descriptor: any) {
    // decorator logic here
}
