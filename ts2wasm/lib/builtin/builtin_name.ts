/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export namespace BuiltinNames {
    // wasm global variable
    export const dataEnd = '~lib/memory/__data_end';
    export const stackPointer = '~lib/memory/__stack_pointer';
    export const heapBase = '~lib/memory/__heap_base';

    // wasm table
    export const extrefTable = 'extref_table';

    // wasm default variable
    export const byteSize = 32;
    export const stackSize = 32768;
    export const memoryOffset = 8;
    export const memInitialPages = 1;
    export const memMaximumPages = 10;
    export const tableInitialPages = 1;
    export const tableMaximumPages = 10;
    export const tableGrowDelta = 10;

    // wasm function
    export const start = '~start';
    export const globalInitFunc = 'global_init';

    // delimiters
    export const moduleDelimiter = '|';

    // import external name
    export const externalModuleName = 'env';

    // builtin module name
    export const builtinModuleName = 'builtin';

    // builtin file name
    export const builtinImplementFileName = 'lib_builtin.ts';
    export const builtinFileNames = ['lib.type.d.ts', builtinImplementFileName];

    // builtin class name
    export const MATH = 'Math';
    export const ARRAY = 'Array';
    export const STRING = 'String';
    export const NUMBER = 'Number';
    export const BOOLEAN = 'Boolean';
    export const OBJECT = 'Object';
    export const FUNCTION = 'Function';
    export const CONSOLE = 'console';

    export const builtinIdentifierArray = [
        MATH,
        ARRAY,
        STRING,
        NUMBER,
        BOOLEAN,
        OBJECT,
        FUNCTION,
        CONSOLE,
    ];

    // decorator name
    export const decorator = 'binaryen';

    // decorator function name
    export const mathSqrtFuncName = 'Math|sqrt';
    export const mathAbsFuncName = 'Math|abs';
    export const mathCeilFuncName = 'Math|ceil';
    export const mathFloorFuncName = 'Math|floor';
    export const mathTruncFuncName = 'Math|trunc';
    export const arrayIsArrayFuncName = 'Array|isArray';
    export const stringConcatFuncName = 'String|concat';
    export const stringSliceFuncName = 'String|slice';

    // builtin instance function name
    export const stringLengthFuncName = 'String|length';
    export const arrayLengthFuncName = 'Array|length';
}

export namespace ArgNames {
    export const opt = 'opt';
    export const disableAny = 'disableAny';
    export const disableBuiltIn = 'disableBuiltIn';
    export const disableInterface = 'disableInterface';
    export const isBuiltIn = 'isBuiltIn';
}
