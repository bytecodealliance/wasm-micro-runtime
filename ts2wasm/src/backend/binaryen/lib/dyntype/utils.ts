/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import binaryen from 'binaryen';

export namespace dyntype {
    // export global dyntype context variable name
    export const dyntype_context = 'dyntype_context';

    // export module name
    export const module_name = 'libdyntype';

    // export type
    export const dyn_ctx_t = binaryen.i64; // binaryen.anyref
    export const dyn_value_t = binaryen.anyref;
    export const dyn_type_t = binaryen.i32;
    export const cvoid = binaryen.none;
    export const double = binaryen.f64;
    export const int = binaryen.i32;
    export const bool = binaryen.i32;
    export const cstring = binaryen.i32;
    export const pointer = binaryen.i32;
    export const external_ref_tag = binaryen.i32;

    // export const
    const module = new binaryen.Module();
    export const bool_true = module.i32.const(1);
    export const bool_false = module.i32.const(0);
    export const DYNTYPE_SUCCESS = module.i32.const(0);
    export const DYNTYPE_EXCEPTION = module.i32.const(1);
    export const DYNTYPE_TYPEERR = module.i32.const(2);

    export const enum ExtObjKind {
        ExtObj = 0,
        ExtFunc = 1,
        ExtInfc = 2,
        ExtArray = 3,
    }

    // export dyntype functions
    export const dyntype_context_init = 'dyntype_context_init';
    export const dyntype_context_init_with_opt =
        'dyntype_context_init_with_opt';
    export const dyntype_context_destroy = 'dyntype_context_destroy';
    export const dyntype_new_number = 'dyntype_new_number';
    export const dyntype_new_boolean = 'dyntype_new_boolean';
    export const dyntype_new_string = 'dyntype_new_string';
    export const dyntype_new_undefined = 'dyntype_new_undefined';
    export const dyntype_new_null = 'dyntype_new_null';
    export const dyntype_new_object = 'dyntype_new_object';
    export const dyntype_new_array = 'dyntype_new_array';
    export const dyntype_new_array_with_length =
        'dyntype_new_array_with_length';
    export const dyntype_add_elem = 'dyntype_add_elem';
    export const dyntype_set_elem = 'dyntype_set_elem';
    export const dyntype_get_elem = 'dyntype_get_elem';
    export const dyntype_new_extref = 'dyntype_new_extref';
    export const dyntype_set_property = 'dyntype_set_property';
    export const dyntype_define_property = 'dyntype_define_property';
    export const dyntype_get_property = 'dyntype_get_property';
    export const dyntype_has_property = 'dyntype_has_property';
    export const dyntype_delete_property = 'dyntype_delete_property';
    export const dyntype_is_undefined = 'dyntype_is_undefined';
    export const dyntype_is_null = 'dyntype_is_null';
    export const dyntype_is_bool = 'dyntype_is_bool';
    export const dyntype_to_bool = 'dyntype_to_bool';
    export const dyntype_is_number = 'dyntype_is_number';
    export const dyntype_to_number = 'dyntype_to_number';
    export const dyntype_is_string = 'dyntype_is_string';
    export const dyntype_to_cstring = 'dyntype_to_cstring';
    export const dyntype_free_cstring = 'dyntype_free_cstring';
    export const dyntype_is_object = 'dyntype_is_object';
    export const dyntype_is_array = 'dyntype_is_array';
    export const dyntype_is_extref = 'dyntype_is_extref';
    export const dyntype_to_extref = 'dyntype_to_extref';
    export const dyntype_typeof = 'dyntype_typeof';
    export const dyntype_type_eq = 'dyntype_type_eq';
    export const dyntype_new_object_with_proto =
        'dyntype_new_object_with_proto';
    export const dyntype_set_prototype = 'dyntype_set_prototype';
    export const dyntype_get_prototype = 'dyntype_get_prototype';
    export const dyntype_get_own_property = 'dyntype_get_own_property';
    export const dyntype_instanceof = 'dyntype_instanceof';
    export const dyntype_dump_value = 'dyntype_dump_value';
    export const dyntype_dump_value_buffer = 'dyntype_dump_value_buffer';
    export const dyntype_hold = 'dyntype_hold';
    export const dyntype_release = 'dyntype_release';
    export const dyntype_collect = 'dyntype_collect';
}

export namespace structdyn {
    export const module_name = 'libstructdyn';
    export const enum StructDyn {
        struct_get_dyn_i32 = 'struct_get_dyn_i32',
        struct_get_dyn_i64 = 'struct_get_dyn_i64',
        struct_get_dyn_f32 = 'struct_get_dyn_f32',
        struct_get_dyn_f64 = 'struct_get_dyn_f64',
        struct_get_dyn_anyref = 'struct_get_dyn_anyref',
        struct_get_dyn_funcref = 'struct_get_dyn_funcref',
        struct_set_dyn_i32 = 'struct_set_dyn_i32',
        struct_set_dyn_i64 = 'struct_set_dyn_i64',
        struct_set_dyn_f32 = 'struct_set_dyn_f32',
        struct_set_dyn_f64 = 'struct_set_dyn_f64',
        struct_set_dyn_anyref = 'struct_set_dyn_anyref',
        struct_set_dyn_funcref = 'struct_set_dyn_funcref',
    }
}
