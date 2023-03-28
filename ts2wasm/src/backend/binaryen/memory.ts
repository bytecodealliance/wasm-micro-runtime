/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import binaryen from 'binaryen';
import { assert } from 'console';
import { BuiltinNames } from '../../../lib/builtin/builtin_name.js';
import Long from 'long';

function i64_new(low: number, high: number) {
    return Long.fromBits(low, high);
}

function i64_add(left: Long, right: Long) {
    return left.add(right);
}

function i64_align(value: Long, alignment: number) {
    assert(alignment && (alignment & (alignment - 1)) == 0);
    const mask = Long.fromInt(alignment - 1);
    return value.add(mask).and(mask.not());
}

function getByteSize(size: number) {
    return (size + 7) >>> 3;
}

function initMemoryOffset() {
    const memoryOffset = Long.fromBits(BuiltinNames.memoryOffset, 0);
    return memoryOffset;
}

export function initGlobalOffset(module: binaryen.Module) {
    let memoryOffset = initMemoryOffset();
    // finalize data
    module.addGlobal(
        BuiltinNames.dataEnd,
        binaryen.i32,
        false,
        module.i32.const(memoryOffset.low),
    );

    // finalize stack
    memoryOffset = i64_align(
        i64_add(memoryOffset, i64_new(BuiltinNames.stackSize, 0)),
        getByteSize(BuiltinNames.byteSize),
    );
    module.addGlobal(
        BuiltinNames.stackPointer,
        binaryen.i32,
        true,
        module.i32.const(memoryOffset.low),
    );

    // finalize heap
    module.addGlobal(
        BuiltinNames.heapBase,
        binaryen.i32,
        false,
        module.i32.const(memoryOffset.low),
    );
}

export function initDefaultMemory(
    module: binaryen.Module,
    segments: binaryen.MemorySegment[],
): void {
    module.setMemory(
        BuiltinNames.memInitialPages,
        BuiltinNames.memMaximumPages,
        'default',
        segments,
    );
}

export function initDefaultTable(module: binaryen.Module): void {
    module.addTable(BuiltinNames.extrefTable, 0, -1, binaryen.anyref);
}
