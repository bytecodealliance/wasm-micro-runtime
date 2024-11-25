/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_log.h"
#include "wasm_export.h"
#include "../common/wasm_runtime_common.h"

/*************************************
 * Functions
 *************************************/

/*************************************
 * Globals
 *************************************/

typedef struct WASMNativeGlobalDef {
    const char *module_name;
    const char *name;
    uint8 type;
    bool is_mutable;
    WASMValue value;
} WASMNativeGlobalDef;

static WASMNativeGlobalDef native_global_defs[] = {
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

wasm_global_inst_t
wasm_native_create_spec_test_builtin_global(wasm_module_t module,
                                            const char *module_name,
                                            const char *name,
                                            wasm_global_type_t type)
{
    if (!module || !module_name || !name || !type) {
        return NULL;
    }

    uint32 size = sizeof(native_global_defs) / sizeof(WASMNativeGlobalDef);
    WASMNativeGlobalDef *global_def = native_global_defs;
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

typedef struct WASMNativeTableDef {
    const char *module_name;
    const char *name;
    uint8 elem_type;

} WASMNativeTableDef;

/*TODO: fix me*/
wasm_table_inst_t *
wasm_native_create_spec_test_builtin_table(wasm_module_t module,
                                           const char *module_name,
                                           const char *name,
                                           wasm_table_type_t type)
{
    if (!module || !module_name || !name || !type) {
        return NULL;
    }

    if (strcmp(module_name, "spectest") != 0) {
        return NULL;
    }

    if (strcmp(name, "table") != 0) {
        return NULL;
    }

    return wasm_runtime_create_table(module, type);
}

/*************************************
 * Memories
 *************************************/

typedef struct WASMNativeMemoryDef {
    const char *module_name;
    const char *name;
} WASMNativeMemoryDef;

/*
 * no predefined memory for spec test
 */
wasm_memory_inst_t
wasm_native_create_spec_test_builtin_memory(wasm_module_t module,
                                            const char *module_name,
                                            const char *name,
                                            wasm_memory_type_t type)
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
