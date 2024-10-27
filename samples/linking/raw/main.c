/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include "wasm_export.h"
#include "bh_read_file.h"

int
main(int argc, char *argv_main[])
{
    int exit_code = EXIT_FAILURE;

    /* runtime */
    if (!wasm_runtime_init()) {
        printf("Init runtime failed.\n");
        return EXIT_FAILURE;
    }

    wasm_runtime_set_log_level(WASM_LOG_LEVEL_VERBOSE);

    /* wasm module file */
    char *buffer;
    uint32 buf_size;
#if WASM_ENABLE_AOT != 0
    printf("Loading AOT file...\n");
    buffer = bh_read_file_to_buffer("import_memory.aot", &buf_size);
#else
    printf("Loading WASM file...\n");
    buffer = bh_read_file_to_buffer("import_memory.wasm", &buf_size);
#endif
    if (!buffer) {
        printf("Open wasm file failed.\n");
        goto destroy_runtime;
    }

    /* wasm module */
    char error_buf[128];
    wasm_module_t module = wasm_runtime_load((uint8 *)buffer, buf_size,
                                             error_buf, sizeof(error_buf));
    if (!module) {
        printf("Load wasm file failed: %s\n", error_buf);
        goto release_file_buffer;
    }

    /* import type */
    int32_t import_count = wasm_runtime_get_import_count(module);
    wasm_import_t import_type = { 0 };
    int32_t import_memory_index = -1;
    for (int i = 0; i < import_count; i++) {
        wasm_runtime_get_import_type(module, i, &import_type);
        if (import_type.kind == WASM_IMPORT_EXPORT_KIND_MEMORY) {
            import_memory_index = i;
            break;
        }
    }

    if (import_memory_index == -1) {
        printf("No memory import found.\n");
        goto unload_module;
    }

    /* host memory */
    wasm_memory_type_t memory_type = import_type.u.memory_type;
    wasm_memory_inst_t memory = wasm_runtime_create_memory(module, memory_type);
    if (!memory) {
        printf("Create memory failed.\n");
        goto unload_module;
    }

    /* import list */
    WASMExternInstance import_list[10] = { 0 };
    import_list[import_memory_index].module_name = "env";
    import_list[import_memory_index].field_name = "memory";
    import_list[import_memory_index].kind = WASM_IMPORT_EXPORT_KIND_MEMORY;
    import_list[import_memory_index].u.memory = memory;

    /* wasm instance */
    InstantiationArgs inst_args = {
        .imports = import_list,
        .import_count = 10,
    };
    wasm_module_inst_t inst = wasm_runtime_instantiate_ex(
        module, &inst_args, error_buf, sizeof(error_buf));
    if (!inst) {
        printf("Instantiate wasm file failed: %s\n", error_buf);
        goto destroy_memory;
    }

    /* export function */
    wasm_function_inst_t func =
        wasm_runtime_lookup_function(inst, "goodhart_law");
    if (!func) {
        printf("The function goodhart_law is not found.\n");
        goto destroy_inst;
    }

    wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(inst, 8192);
    if (!exec_env) {
        printf("Create wasm execution environment failed.\n");
        goto destroy_inst;
    }

    if (!wasm_runtime_call_wasm(exec_env, func, 0, NULL)) {
        printf("call wasm function goodhart_law failed. %s\n",
               wasm_runtime_get_exception(inst));
        goto destroy_exec_env;
    }

    exit_code = EXIT_SUCCESS;

destroy_exec_env:
    wasm_runtime_destroy_exec_env(exec_env);
destroy_inst:
    wasm_runtime_deinstantiate(inst);
destroy_memory:
    wasm_runtime_destroy_memory(module, memory);
unload_module:
    wasm_runtime_unload(module);
release_file_buffer:
    wasm_runtime_free(buffer);
destroy_runtime:
    wasm_runtime_destroy();

    return exit_code;
}