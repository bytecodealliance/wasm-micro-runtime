/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/* eslint-disable @typescript-eslint/no-empty-interface */
/* eslint-disable @typescript-eslint/ban-types */
/* eslint-disable no-var */

interface ConcatArray<T> {
    readonly length: number;
    readonly [n: number]: T;
    join(separator?: string): string;
    slice(start?: number, end?: number): T[];
}

interface ArrayConstructor {
    new (arrayLength?: number): any[];
    new <T>(arrayLength: number): T[];
    new <T>(...items: T[]): T[];
    (arrayLength?: number): any[];
    <T>(arrayLength: number): T[];
    <T>(...items: T[]): T[];
    isArray(arg: any): arg is any[];
}
var Array: ArrayConstructor;

interface Array<T> {
    length: number;
    slice(start?: number, end?: number): T[];
    concat(...items: ConcatArray<T>[]): T[];
    concat(...items: (T | ConcatArray<T>)[]): T[];
    [n: number]: T;
}

interface Boolean {}

interface Number {}

interface Function {}

type CallableFunction = Function;

interface IArguments {
    [index: number]: any;
}

type NewableFunction = Function;

interface Object {}

interface RegExp {}

interface String {
    readonly length: number;
    concat(...strings: string[]): string;
    slice(start?: number, end?: number): string;
    readonly [index: number]: string;
}

interface StringConstructor {
    new (value?: any): String;
    (value?: any): string;
    readonly prototype: String;
    fromCharCode(...codes: number[]): string;
}
var String: StringConstructor;

interface Math {
    pow(x: number, y: number): number;
    max(...values: number[]): number;
    min(...values: number[]): number;
    sqrt(x: number): number;
    abs(x: number): number;
    ceil(x: number): number;
    floor(x: number): number;
}
var Math: Math;

interface TypedPropertyDescriptor<T> {}

interface Console {
    log(...data: any[]): void;
}
var console: Console;
