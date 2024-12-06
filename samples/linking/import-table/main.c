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
    buffer = bh_read_file_to_buffer("import_table.aot", &buf_size);
#else
    printf("Loading WASM file...\n");
    buffer = bh_read_file_to_buffer("import_table.wasm", &buf_size);
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
    if (import_count < 0)
        goto unload_module;

    wasm_import_t import_type = { 0 };
    int32_t import_table_index = -1;
    for (int32_t i = 0; i < import_count; i++) {
        wasm_runtime_get_import_type(module, (uint32_t)i, &import_type);
        if (import_type.kind == WASM_IMPORT_EXPORT_KIND_TABLE) {
            import_table_index = i;
            break;
        }
    }

    if (import_table_index == -1) {
        printf("No memory import found.\n");
        goto unload_module;
    }

    /* host table */
    wasm_table_type_t table_type = import_type.u.table_type;
    wasm_table_inst_t *table = wasm_runtime_create_table(module, table_type);
    if (!table) {
        printf("Create table failed.\n");
        goto unload_module;
    }

    /* import list */
    WASMExternInstance import_list[10] = { 0 };
    import_list[import_table_index].module_name = "host";
    import_list[import_table_index].field_name = "__indirect_function_table";
    import_list[import_table_index].kind = WASM_IMPORT_EXPORT_KIND_TABLE;
    import_list[import_table_index].u.table = table;

    /* wasm instance */
    InstantiationArgs inst_args = {
        .imports = import_list,
        .import_count = 10,
    };
    wasm_module_inst_t inst = wasm_runtime_instantiate_ex(
        module, &inst_args, error_buf, sizeof(error_buf));
    if (!inst) {
        printf("Instantiate wasm file failed: %s\n", error_buf);
        goto destroy_table;
    }

    /* export function */
    if (!wasm_application_execute_main(inst, 0, NULL)) {
        const char *exception = wasm_runtime_get_exception(inst);
        printf("call wasm function main() failed. %s\n", exception);
        goto destroy_inst;
    }

    exit_code = EXIT_SUCCESS;

destroy_inst:
    wasm_runtime_deinstantiate(inst);
destroy_table:
    wasm_runtime_destroy_table(module, table);
unload_module:
    wasm_runtime_unload(module);
release_file_buffer:
    wasm_runtime_free(buffer);
destroy_runtime:
    wasm_runtime_destroy();

    return exit_code;
}