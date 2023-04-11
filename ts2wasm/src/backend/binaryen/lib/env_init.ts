/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

import binaryen from 'binaryen';
import { dyntype, structdyn } from './dyntype/utils.js';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { addWatFuncs } from '../utils.js';

export function importAnyLibAPI(module: binaryen.Module) {
    module.addFunctionImport(
        dyntype.dyntype_context_init,
        dyntype.module_name,
        dyntype.dyntype_context_init,
        binaryen.none,
        dyntype.dyn_ctx_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_context_destroy,
        dyntype.module_name,
        dyntype.dyntype_context_destroy,
        dyntype.dyn_ctx_t,
        dyntype.cvoid,
    );
    module.addFunctionImport(
        dyntype.dyntype_new_number,
        dyntype.module_name,
        dyntype.dyntype_new_number,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.double]),
        dyntype.dyn_value_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_new_string,
        dyntype.module_name,
        dyntype.dyntype_new_string,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.dyn_value_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_new_boolean,
        dyntype.module_name,
        dyntype.dyntype_new_boolean,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.bool]),
        dyntype.dyn_value_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_typeof,
        dyntype.module_name,
        dyntype.dyntype_typeof,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.dyn_type_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_type_eq,
        dyntype.module_name,
        dyntype.dyntype_type_eq,
        binaryen.createType([
            dyntype.dyn_ctx_t,
            dyntype.dyn_value_t,
            dyntype.dyn_value_t,
        ]),
        dyntype.bool,
    );
    module.addFunctionImport(
        dyntype.dyntype_is_number,
        dyntype.module_name,
        dyntype.dyntype_is_number,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.bool,
    );
    module.addFunctionImport(
        dyntype.dyntype_to_number,
        dyntype.module_name,
        dyntype.dyntype_to_number,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.double,
    );
    module.addFunctionImport(
        dyntype.dyntype_is_undefined,
        dyntype.module_name,
        dyntype.dyntype_is_undefined,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.bool,
    );
    module.addFunctionImport(
        dyntype.dyntype_new_undefined,
        dyntype.module_name,
        dyntype.dyntype_new_undefined,
        dyntype.dyn_ctx_t,
        dyntype.dyn_value_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_new_null,
        dyntype.module_name,
        dyntype.dyntype_new_null,
        dyntype.dyn_ctx_t,
        dyntype.dyn_value_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_new_object,
        dyntype.module_name,
        dyntype.dyntype_new_object,
        dyntype.dyn_ctx_t,
        dyntype.dyn_value_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_new_array,
        dyntype.module_name,
        dyntype.dyntype_new_array,
        dyntype.dyn_ctx_t,
        dyntype.dyn_value_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_new_array_with_length,
        dyntype.module_name,
        dyntype.dyntype_new_array_with_length,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.int]),
        dyntype.dyn_value_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_add_elem,
        dyntype.module_name,
        dyntype.dyntype_add_elem,
        binaryen.createType([
            dyntype.dyn_ctx_t,
            dyntype.dyn_value_t,
            dyntype.dyn_value_t,
        ]),
        dyntype.cvoid,
    );
    module.addFunctionImport(
        dyntype.dyntype_set_elem,
        dyntype.module_name,
        dyntype.dyntype_set_elem,
        binaryen.createType([
            dyntype.dyn_ctx_t,
            dyntype.dyn_value_t,
            dyntype.int,
            dyntype.dyn_value_t,
        ]),
        dyntype.cvoid,
    );
    module.addFunctionImport(
        dyntype.dyntype_get_elem,
        dyntype.module_name,
        dyntype.dyntype_get_elem,
        binaryen.createType([
            dyntype.dyn_ctx_t,
            dyntype.dyn_value_t,
            dyntype.int,
        ]),
        dyntype.dyn_value_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_is_array,
        dyntype.module_name,
        dyntype.dyntype_is_array,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.bool,
    );
    module.addFunctionImport(
        dyntype.dyntype_set_property,
        dyntype.module_name,
        dyntype.dyntype_set_property,
        binaryen.createType([
            dyntype.dyn_ctx_t,
            dyntype.dyn_value_t,
            dyntype.cstring,
            dyntype.dyn_value_t,
        ]),
        dyntype.bool,
    );
    module.addFunctionImport(
        dyntype.dyntype_get_property,
        dyntype.module_name,
        dyntype.dyntype_get_property,
        binaryen.createType([
            dyntype.dyn_ctx_t,
            dyntype.dyn_value_t,
            dyntype.cstring,
        ]),
        dyntype.dyn_value_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_has_property,
        dyntype.module_name,
        dyntype.dyntype_has_property,
        binaryen.createType([
            dyntype.dyn_ctx_t,
            dyntype.dyn_value_t,
            dyntype.cstring,
        ]),
        dyntype.int,
    );
    module.addFunctionImport(
        dyntype.dyntype_delete_property,
        dyntype.module_name,
        dyntype.dyntype_delete_property,
        binaryen.createType([
            dyntype.dyn_ctx_t,
            dyntype.dyn_value_t,
            dyntype.cstring,
        ]),
        dyntype.int,
    );
    module.addFunctionImport(
        dyntype.dyntype_new_extref,
        dyntype.module_name,
        dyntype.dyntype_new_extref,
        binaryen.createType([
            dyntype.dyn_ctx_t,
            dyntype.pointer,
            dyntype.external_ref_tag,
        ]),
        dyntype.dyn_value_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_is_extref,
        dyntype.module_name,
        dyntype.dyntype_is_extref,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.bool,
    );
    module.addFunctionImport(
        dyntype.dyntype_to_extref,
        dyntype.module_name,
        dyntype.dyntype_to_extref,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.int,
    );
    module.addFunctionImport(
        dyntype.dyntype_is_object,
        dyntype.module_name,
        dyntype.dyntype_is_object,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.bool,
    );
    module.addFunctionImport(
        dyntype.dyntype_get_prototype,
        dyntype.module_name,
        dyntype.dyntype_get_prototype,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.dyn_value_t,
    );
    module.addFunctionImport(
        dyntype.dyntype_set_prototype,
        dyntype.module_name,
        dyntype.dyntype_set_prototype,
        binaryen.createType([
            dyntype.dyn_ctx_t,
            dyntype.dyn_value_t,
            dyntype.dyn_value_t,
        ]),
        dyntype.int,
    );
    module.addFunctionImport(
        dyntype.dyntype_is_bool,
        dyntype.module_name,
        dyntype.dyntype_is_bool,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.bool,
    );
    module.addFunctionImport(
        dyntype.dyntype_to_bool,
        dyntype.module_name,
        dyntype.dyntype_to_bool,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.bool,
    );
    module.addFunctionImport(
        dyntype.dyntype_is_string,
        dyntype.module_name,
        dyntype.dyntype_is_string,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.bool,
    );
    module.addFunctionImport(
        dyntype.dyntype_to_cstring,
        dyntype.module_name,
        dyntype.dyntype_to_cstring,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.dyn_value_t]),
        dyntype.int,
    );
    module.addFunctionImport(
        dyntype.dyntype_free_cstring,
        dyntype.module_name,
        dyntype.dyntype_free_cstring,
        binaryen.createType([dyntype.dyn_ctx_t, dyntype.pointer]),
        dyntype.cvoid,
    );
}

export function importInfcLibAPI(module: binaryen.Module) {
    module.addFunctionImport(
        structdyn.StructDyn.struct_get_dyn_i32,
        structdyn.module_name,
        structdyn.StructDyn.struct_get_dyn_i32,
        binaryen.createType([binaryen.anyref, binaryen.i32]),
        binaryen.i32,
    );

    module.addFunctionImport(
        structdyn.StructDyn.struct_get_dyn_i64,
        structdyn.module_name,
        structdyn.StructDyn.struct_get_dyn_i64,
        binaryen.createType([binaryen.anyref, binaryen.i32]),
        binaryen.i64,
    );

    module.addFunctionImport(
        structdyn.StructDyn.struct_get_dyn_f32,
        structdyn.module_name,
        structdyn.StructDyn.struct_get_dyn_f32,
        binaryen.createType([binaryen.anyref, binaryen.i32]),
        binaryen.f32,
    );

    module.addFunctionImport(
        structdyn.StructDyn.struct_get_dyn_f64,
        structdyn.module_name,
        structdyn.StructDyn.struct_get_dyn_f64,
        binaryen.createType([binaryen.anyref, binaryen.i32]),
        binaryen.f64,
    );

    module.addFunctionImport(
        structdyn.StructDyn.struct_get_dyn_anyref,
        structdyn.module_name,
        structdyn.StructDyn.struct_get_dyn_anyref,
        binaryen.createType([binaryen.anyref, binaryen.i32]),
        binaryen.anyref,
    );

    module.addFunctionImport(
        structdyn.StructDyn.struct_get_dyn_funcref,
        structdyn.module_name,
        structdyn.StructDyn.struct_get_dyn_funcref,
        binaryen.createType([binaryen.anyref, binaryen.i32]),
        binaryen.funcref,
    );

    module.addFunctionImport(
        structdyn.StructDyn.struct_set_dyn_i32,
        structdyn.module_name,
        structdyn.StructDyn.struct_set_dyn_i32,
        binaryen.createType([binaryen.anyref, binaryen.i32, binaryen.i32]),
        binaryen.none,
    );

    module.addFunctionImport(
        structdyn.StructDyn.struct_set_dyn_i64,
        structdyn.module_name,
        structdyn.StructDyn.struct_set_dyn_i64,
        binaryen.createType([binaryen.anyref, binaryen.i32, binaryen.i64]),
        binaryen.none,
    );

    module.addFunctionImport(
        structdyn.StructDyn.struct_set_dyn_f32,
        structdyn.module_name,
        structdyn.StructDyn.struct_set_dyn_f32,
        binaryen.createType([binaryen.anyref, binaryen.i32, binaryen.f32]),
        binaryen.none,
    );

    module.addFunctionImport(
        structdyn.StructDyn.struct_set_dyn_f64,
        structdyn.module_name,
        structdyn.StructDyn.struct_set_dyn_f64,
        binaryen.createType([binaryen.anyref, binaryen.i32, binaryen.f64]),
        binaryen.none,
    );

    module.addFunctionImport(
        structdyn.StructDyn.struct_set_dyn_anyref,
        structdyn.module_name,
        structdyn.StructDyn.struct_set_dyn_anyref,
        binaryen.createType([binaryen.anyref, binaryen.i32, binaryen.anyref]),
        binaryen.none,
    );

    module.addFunctionImport(
        structdyn.StructDyn.struct_set_dyn_funcref,
        structdyn.module_name,
        structdyn.StructDyn.struct_set_dyn_anyref,
        binaryen.createType([binaryen.anyref, binaryen.i32, binaryen.funcref]),
        binaryen.none,
    );
}

export function generateGlobalContext(module: binaryen.Module) {
    module.addGlobal(
        dyntype.dyntype_context,
        dyntype.dyn_ctx_t,
        true,
        module.i64.const(0, 0),
    );
}

export function generateInitDynContext(module: binaryen.Module) {
    const initDynContextStmt = module.global.set(
        dyntype.dyntype_context,
        module.call(dyntype.dyntype_context_init, [], binaryen.none),
    );

    return initDynContextStmt;
}

export function generateFreeDynContext(module: binaryen.Module) {
    const freeDynContextStmt = module.call(
        dyntype.dyntype_context_destroy,
        [module.global.get(dyntype.dyntype_context, dyntype.dyn_ctx_t)],
        binaryen.none,
    );

    return freeDynContextStmt;
}

export function addItableFunc(module: binaryen.Module) {
    /* add find_index function from .wat */
    /* TODO: Have not found an effiective way to load import function from .wat yet */
    module.addFunctionImport(
        'strcmp',
        'env',
        'strcmp',
        binaryen.createType([binaryen.i32, binaryen.i32]),
        binaryen.i32,
    );
    const itableFilePath = path.join(
        path.dirname(fileURLToPath(import.meta.url)),
        'interface',
        'itable.wat',
    );
    const itableLib = fs.readFileSync(itableFilePath, 'utf-8');
    const watModule = binaryen.parseText(itableLib);
    addWatFuncs(watModule, 'find_index', module);
    watModule.dispose();
}
