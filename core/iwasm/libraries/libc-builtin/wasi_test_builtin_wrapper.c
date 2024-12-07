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

/*************************************
 * Tables
 *************************************/

/*************************************
 * Memories
 *************************************/

static WASMNativeMemoryDef builtin_memory_defs[] = {
    { "foo", "bar" },
    { "env", "memory" },
};

static wasm_memory_inst_t
create_wasi_test_memory(wasm_module_t module, const char *module_name,
                        const char *name, wasm_memory_type_t type)
{
    if (!module || !module_name || !name || !type) {
        return NULL;
    }

    WASMNativeMemoryDef *memory_def = builtin_memory_defs;
    size_t count = sizeof(builtin_memory_defs) / sizeof(WASMNativeMemoryDef);
    WASMNativeMemoryDef *memory_def_end = builtin_memory_defs + count;

    for (; memory_def < memory_def_end; memory_def++) {
        if (strcmp(memory_def->module_name, module_name) != 0) {
            continue;
        }

        if (strcmp(memory_def->name, name) != 0) {
            continue;
        }

        return wasm_runtime_create_memory(module, type);
    }

    return NULL;
}

/*************************************
 * Extern
 *************************************/

bool
wasm_runtime_create_extern_inst_for_wasi_test(wasm_module_t module,
                                              wasm_import_t *import_type,
                                              WASMExternInstance *out)
{
    if (!module || !import_type || !out)
        return false;

    LOG_DEBUG("create import(%s,%s) kind %d", import_type->module_name,
              import_type->name, import_type->kind);

    out->module_name = import_type->module_name;
    out->field_name = import_type->name;
    out->kind = import_type->kind;

    if (import_type->kind != WASM_IMPORT_EXPORT_KIND_MEMORY) {
        LOG_DEBUG("unimplemented import(%s,%s) kind %d for wasi test",
                  import_type->module_name, import_type->name,
                  import_type->kind);
        return true;
    }

    out->u.memory =
        create_wasi_test_memory(module, import_type->module_name,
                                import_type->name, import_type->u.memory_type);
    if (!out->u.memory) {
        LOG_ERROR("create memory failed\n");
        return false;
    }

    return true;
}