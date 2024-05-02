/*
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_export.h"
#include "bh_read_file.h"

void
print_usage(void)
{
    fprintf(stdout, "Options:\r\n");
    fprintf(stdout, "  -f [path of wasm file] \n");
}

int
main(int argc, char *argv_main[])
{
    static char global_heap_buf[512 * 1024];
    char *buffer, error_buf[128];
    int opt;
    char *wasm_path = NULL;
    bool success;

    wasm_module_t module = NULL;
    wasm_module_inst_t module_inst = NULL;
    uint32 buf_size, stack_size = 8092, heap_size = 8092;
    load_section_result_t sections = { 0 };

    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    while ((opt = getopt(argc, argv_main, "hf:")) != -1) {
        switch (opt) {
            case 'f':
                wasm_path = optarg;
                break;
            case 'h':
                print_usage();
                return 0;
            case '?':
                print_usage();
                return 0;
        }
    }
    if (optind == 1) {
        print_usage();
        return 0;
    }

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        return -1;
    }

    buffer = bh_read_file_to_buffer(wasm_path, &buf_size);
    if (!buffer) {
        printf("Open wasm app file [%s] failed.\n", wasm_path);
        goto fail;
    }

    success = wasm_runtime_read_to_sections(
        (uint8 *)buffer, buf_size, &sections, error_buf, sizeof(error_buf));
    if (!success) {
        printf("Read wasm sections failed: %s.\n", error_buf);
        goto fail;
    }
    wasm_runtime_free(buffer);
    buffer = NULL;

    module = wasm_runtime_load_from_sections(sections.sections, sections.is_aot,
                                             error_buf, sizeof(error_buf));
    if (!module) {
        printf("Load from sections failed: %s.\n", error_buf);
        goto fail;
    }

    /* Destroy some sections after loading */
    if (sections.is_aot) {
        uint8 sections_to_free[] = {
            WASM_AOT_SECTION_TYPE_TARGET_INFO, WASM_AOT_SECTION_TYPE_INIT_DATA,
            WASM_AOT_SECTION_TYPE_FUNCTION,    WASM_AOT_SECTION_TYPE_EXPORT,
            WASM_AOT_SECTION_TYPE_RELOCATION,  WASM_AOT_SECTION_TYPE_SIGANATURE,
            WASM_AOT_SECTION_TYPE_CUSTOM,
        };
        wasm_runtime_destroy_sections(&sections, sections_to_free,
                                      sizeof(sections_to_free)
                                          / sizeof(uint8_t));
    }
    else {
        uint8_t sections_to_free[] = {
            WASM_SECTION_TYPE_USER,     WASM_SECTION_TYPE_TYPE,
            WASM_SECTION_TYPE_IMPORT,   WASM_SECTION_TYPE_FUNC,
            WASM_SECTION_TYPE_TABLE,    WASM_SECTION_TYPE_MEMORY,
            WASM_SECTION_TYPE_GLOBAL,   WASM_SECTION_TYPE_EXPORT,
            WASM_SECTION_TYPE_START,    WASM_SECTION_TYPE_ELEM,
            WASM_SECTION_TYPE_DATACOUNT
        };
        wasm_runtime_destroy_sections(&sections, sections_to_free,
                                      sizeof(sections_to_free)
                                          / sizeof(uint8_t));
    }

    module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                           error_buf, sizeof(error_buf));
    if (!module_inst) {
        printf("Instantiate wasm module failed. error: %s.\n", error_buf);
        goto fail;
    }

    /* Destroy all sections after instantiation. For AOT, the text sections must
     * be kept because referenced during execution */
    if (!sections.is_aot) {
        wasm_runtime_destroy_sections(&sections, NULL, 0);
    }

    char *args[1] = { "3" };
    success = wasm_application_execute_func(module_inst, "mul7", 1, args);
    if (!success) {
        printf("Unable to execute function.\n");
        goto fail;
    }

fail:
    if (module_inst)
        wasm_runtime_deinstantiate(module_inst);
    if (module)
        wasm_runtime_unload(module);
    if (buffer)
        BH_FREE(buffer);
    wasm_runtime_destroy();
    return 0;
}
