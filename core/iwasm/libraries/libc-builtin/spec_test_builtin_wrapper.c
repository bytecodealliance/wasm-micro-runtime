/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_log.h"
#include "builtin_wrapper.h"

/*************************************
 * Functions
 *************************************/

/*************************************
 * Globals
 *************************************/

static WASMNativeGlobalDef spec_test_global_defs[] = {
    /* for standard spec test */
    { "spectest", "global_i32", VALUE_TYPE_I32, false, .value.i32 = 666 },
    { "spectest", "global_i64", VALUE_TYPE_I64, false, .value.i64 = 666 },
    { "spectest", "global_f32", VALUE_TYPE_F32, false, .value.f32 = 666.6f },
    { "spectest", "global_f64", VALUE_TYPE_F64, false, .value.f64 = 666.6 },
    { "test", "global-i32", VALUE_TYPE_I32, false, .value.i32 = 0 },
    { "test", "global-f32", VALUE_TYPE_F32, false, .value.f32 = 0 },
    { "test", "global-mut-i32", VALUE_TYPE_I32, true, .value.i32 = 0 },
    { "test", "global-mut-i64", VALUE_TYPE_I64, true, .value.i64 = 0 },
    { "test", "g", VALUE_TYPE_I32, true, .value.i32 = 0 },
/* for gc spec test */
#if WASM_ENABLE_GC != 0
    { "G", "g", VALUE_TYPE_I32, false, .value.i32 = 4 },
    { "M", "g", REF_TYPE_HT_NON_NULLABLE, false, .value.gc_obj = 0 },
#endif
};

static wasm_global_inst_t
create_spec_test_global(wasm_module_t module, const char *module_name,
                        const char *name, wasm_global_type_t type)
{
    if (!module || !module_name || !name || !type) {
        return NULL;
    }

    WASMNativeGlobalDef *global_def = spec_test_global_defs;
    uint32 size = sizeof(spec_test_global_defs) / sizeof(WASMNativeGlobalDef);
    WASMNativeGlobalDef *global_def_end = global_def + size;
    for (; global_def < global_def_end; global_def++) {
        if (strcmp(global_def->module_name, module_name) != 0) {
            continue;
        }

        if (strcmp(global_def->name, name) != 0) {
            continue;
        }

        wasm_global_inst_t global =
            wasm_runtime_create_global_internal(module, NULL, type);
        if (!global) {
            return NULL;
        }

        wasm_runtime_set_global_value(module, global, &global_def->value);
        return global;
    }

    return NULL;
}

/*************************************
 * Tables
 *************************************/
static WASMNativeTableDef builtin_table_defs[] = {
    { "spectest", "table", VALUE_TYPE_FUNCREF },
    { "spectest", "table64", VALUE_TYPE_FUNCREF },
};

static wasm_table_inst_t
create_spec_test_table(wasm_module_t module, const char *module_name,
                       const char *name, wasm_table_type_t type)
{
    if (!module || !module_name || !name || !type) {
        return NULL;
    }

    WASMNativeTableDef *table_def = builtin_table_defs;
    size_t count = sizeof(builtin_table_defs) / sizeof(WASMNativeTableDef);
    WASMNativeTableDef *table_def_end = builtin_table_defs + count;

    for (; table_def < table_def_end; table_def++) {
        if (strcmp(table_def->module_name, module_name) != 0) {
            continue;
        }

        if (strcmp(table_def->name, name) != 0) {
            continue;
        }

        return wasm_runtime_create_table(module, type);
    }

    return NULL;
}

/*************************************
 * Memories
 *************************************/

/*
 * no predefined memory for spec test
 */
static wasm_memory_inst_t
create_spec_test_memory(wasm_module_t module, const char *module_name,
                        const char *name, wasm_memory_type_t type)
{
    if (!module || !module_name || !name || !type) {
        return NULL;
    }

    if (strcmp(module_name, "spectest") != 0) {
        return NULL;
    }

    if (strcmp(name, "memory") != 0) {
        return NULL;
    }

    return wasm_runtime_create_memory(module, type);
}

/*************************************
 * Extern
 *************************************/

bool
wasm_runtime_create_extern_inst_for_spec_test(wasm_module_t module,
                                              wasm_import_t *import_type,
                                              WASMExternInstance *out)
{
    if (!module || !import_type || !out)
        return false;

    if (import_type->kind == WASM_IMPORT_EXPORT_KIND_FUNC) {
        /* Let wasm_native inject wrappers into WASMFunctionImport */
        LOG_DEBUG("skip import(%s,%s) kind %d", import_type->module_name,
                  import_type->name, import_type->kind);
        return true;
    }

    LOG_DEBUG("create import(%s,%s) kind %d", import_type->module_name,
              import_type->name, import_type->kind);

    out->module_name = import_type->module_name;
    out->field_name = import_type->name;
    out->kind = import_type->kind;

    if (import_type->kind == WASM_IMPORT_EXPORT_KIND_MEMORY) {
        out->u.memory = create_spec_test_memory(
            module, import_type->module_name, import_type->name,
            import_type->u.memory_type);
        if (!out->u.memory) {
            LOG_ERROR("create memory failed\n");
            return false;
        }
    }
    else if (import_type->kind == WASM_IMPORT_EXPORT_KIND_TABLE) {
        out->u.table = create_spec_test_table(module, import_type->module_name,
                                              import_type->name,
                                              import_type->u.table_type);
        if (!out->u.table) {
            LOG_ERROR("create table failed\n");
            return false;
        }
    }
    else if (import_type->kind == WASM_IMPORT_EXPORT_KIND_GLOBAL) {
        out->u.global = create_spec_test_global(
            module, import_type->module_name, import_type->name,
            import_type->u.global_type);
        if (!out->u.global) {
            LOG_ERROR("create global failed\n");
            return false;
        }
    }
    else {
        LOG_DEBUG("unimplemented import(%s,%s) kind %d for spec test",
                  import_type->module_name, import_type->name,
                  import_type->kind);
    }

    return true;
}
