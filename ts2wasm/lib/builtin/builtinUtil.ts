/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export namespace BuiltinNames {
    // wasm global variable
    export const data_end = '~lib/memory/__data_end';
    export const stack_pointer = '~lib/memory/__stack_pointer';
    export const heap_base = '~lib/memory/__heap_base';

    // wasm table
    export const extref_table = 'extref_table';

    // wasm default variable
    export const byteSize = 32;
    export const stackSize = 32768;
    export const memoryOffset = 8;
    export const mem_initialPages = 1;
    export const mem_maximumPages = 10;
    export const table_initialPages = 1;
    export const table_maximumPages = 10;

    // wasm function
    export const start = '~start';

    // delimiters
    export const module_delimiter = '|';

    // import external name
    export const external_module_name = 'env';

    export const bulitIn_module_name = 'builtIn';

    // decorator name
    export const decorator = 'binaryen';

    // Math builtin function
    export const MATH = 'Math';
    export const ISARRAY = 'isArray';
    export const Math_sqrt_funcName = 'Math|sqrt';
    export const Math_abs_funcName = 'Math|abs';
    export const Math_ceil_funcName = 'Math|ceil';
    export const Math_floor_funcName = 'Math|floor';
    export const Math_trunc_funcName = 'Math|trunc';
    export const Math_max_funcName = 'Math|max';
    export const Math_min_funcName = 'Math|min';
    export const Math_pow_funcName = 'Math|pow';

    export const MathBuiltInFuncs: {
        [key: string]: string;
    } = {
        sqrt: Math_sqrt_funcName,
        abs: Math_abs_funcName,
        ceil: Math_ceil_funcName,
        floor: Math_floor_funcName,
        trunc: Math_trunc_funcName,
        max: Math_max_funcName,
        min: Math_min_funcName,
        pow: Math_pow_funcName,
    };

    // Array builtin function
    export const ARRAY = 'Array';
    export const Array_of_funcName = 'Array|of';
    export const Array_isArray_funcName = 'Array|isArray';
    export const Array_from_funcName = 'Array|from';

    export const ArrayBuiltInFuncs: {
        [key: string]: string;
    } = {
        of: Array_of_funcName,
        isArray: Array_isArray_funcName,
        from: Array_from_funcName,
    };

    // console builtin function
    export const console_log_funcName = 'console_log';

    export const consoleBuiltInFuncs: {
        [key: string]: string;
    } = {
        log: console_log_funcName,
    };

    // typescript built_in identifier list
    export const builtInObjInfo: {
        [key: string]: { [key: string]: string };
    } = {
        Math: MathBuiltInFuncs,
        Array: ArrayBuiltInFuncs,
        console: consoleBuiltInFuncs,
    };

    // typescript built_in function names
    export const CONCAT = 'concat';
    export const AT = 'at';
    export const FIND = 'find';
    export const FINDINDEX = 'findIndex';
    export const INCLUDES = 'includes';
    export const INDEXOF = 'indexOf';
    export const POP = 'pop';
    export const PUSH = 'push';
    export const SHIFT = 'shift';
    export const SLICE = 'slice';
    export const TOFIXED = 'toFixed';
    export const VALUEOF = 'valueOf';
    export const BIND = 'bind';
    export const CALL = 'call';
    export const APPLY = 'apply';

    // string builtin function
    export const string_length_funcName = 'lib|string_length';
    export const string_concat_funcName = 'lib|string_concat';
    export const string_slice_funcName = 'lib|string_slice';

    // number builtin function
    export const number_toFixed_funcName = 'lib|number_toFixed';

    // boolean builtin function
    export const boolean_valueOf_funcName = 'lib|boolean_valueOf';

    // func builtin function
    export const func_bind_funcName = 'lib|func_bind';
    export const func_call_funcName = 'lib|func_call';
    export const func_apply_funcName = 'lib|func_apply';

    // array builtIn function name
    export const array_length_funcName = 'lib|array_length';
    export const array_concat_funcName = 'lib|array_concat';
    export const array_at_funcName = 'lib|array_at';
    export const array_find_funcName = 'lib|array_find';
    export const array_findIndex_funcName = 'lib|array_findIndex';
    export const array_includes_funcName = 'lib|array_includes';
    export const array_indexOf_funcName = 'lib|array_indexOf';
    export const array_pop_funcName = 'lib|array_pop';
    export const array_push_funcName = 'lib|array_push';
    export const array_shift_funcName = 'lib|array_shift';
    export const array_slice_funcName = 'lib|array_slice';

    export const arrayBuiltInFuncs: {
        [key: string]: string;
    } = {
        length: array_length_funcName,
        concat: array_concat_funcName,
        at: array_at_funcName,
        find: array_find_funcName,
        findIndex: array_findIndex_funcName,
        includes: array_includes_funcName,
        indexOf: array_indexOf_funcName,
        pop: array_pop_funcName,
        push: array_push_funcName,
        shift: array_shift_funcName,
        slice: array_slice_funcName,
    };

    export const numberBuiltInFuncs: {
        [key: string]: string;
    } = {
        TOFIXED: number_toFixed_funcName,
    };

    export const stringBuiltInFuncs: {
        [key: string]: string;
    } = {
        concat: string_concat_funcName,
        slice: string_slice_funcName,
        length: string_length_funcName,
    };

    export const booleanBuiltInFuncs: {
        [key: string]: string;
    } = {
        valueof: boolean_valueOf_funcName,
    };

    export const funcBuiltInFuncs: {
        [key: string]: string;
    } = {
        call: func_call_funcName,
        bind: func_bind_funcName,
        apply: func_apply_funcName,
    };

    export const builtInInstInfo: {
        [key: string]: { [key: string]: string };
    } = {
        array: arrayBuiltInFuncs,
        number: numberBuiltInFuncs,
        string: stringBuiltInFuncs,
        boolean: booleanBuiltInFuncs,
        function: funcBuiltInFuncs,
    };
}

export namespace ArgNames {
    export const opt = 'opt';
    export const disableAny = 'disableAny';
    export const disableBuiltIn = 'disableBuiltIn';
    export const disableInterface = 'disableInterface';
    export const isBuiltIn = 'isBuiltIn';
}
