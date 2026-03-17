/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_getopt.h"
#include "bh_read_file.h"
#include "wasm_export.h"

int32_t
get_custom_section_handle(wasm_exec_env_t exec_env, const char *section_name);
void
print_custom_section(wasm_exec_env_t exec_env, int32_t handle);
void
reset_custom_section_handles(void);

static void
print_usage(void)
{
    fprintf(stdout, "Options:\r\n");
    fprintf(stdout, "  -f [path of wasm file]\n");
}

int
main(int argc, char *argv_main[])
{
    static char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
    wasm_module_t module = NULL;
    wasm_module_inst_t module_inst = NULL;
    wasm_exec_env_t exec_env = NULL;
    wasm_function_inst_t func = NULL;
    wasm_val_t results[1] = { { .kind = WASM_I32, .of.i32 = -1 } };
    char *buffer = NULL;
    char *wasm_path = NULL;
    char error_buf[128];
    int exit_code = 1;
    int opt;
    uint32_t buf_size;
    uint32_t stack_size = 8092;
    uint32_t heap_size = 8092;

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

    static NativeSymbol native_symbols[] = {
        { "get_custom_section_handle", get_custom_section_handle, "($)i",
          NULL },
        { "print_custom_section", print_custom_section, "(i)", NULL },
    };

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
    init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
    init_args.native_module_name = "env";
    init_args.native_symbols = native_symbols;

    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        return -1;
    }

    reset_custom_section_handles();

    buffer = bh_read_file_to_buffer(wasm_path, &buf_size);
    if (!buffer) {
        printf("Open wasm app file [%s] failed.\n", wasm_path);
        goto fail;
    }

    module = wasm_runtime_load((uint8 *)buffer, buf_size, error_buf,
                               sizeof(error_buf));
    if (!module) {
        printf("Load wasm module failed. error: %s\n", error_buf);
        goto fail;
    }

    module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                           error_buf, sizeof(error_buf));
    if (!module_inst) {
        printf("Instantiate wasm module failed. error: %s\n", error_buf);
        goto fail;
    }

    exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
    if (!exec_env) {
        printf("Create wasm execution environment failed.\n");
        goto fail;
    }

    func = wasm_runtime_lookup_function(module_inst, "run_demo");
    if (!func) {
        printf("The wasm function run_demo is not found.\n");
        goto fail;
    }

    if (!wasm_runtime_call_wasm_a(exec_env, func, 1, results, 0, NULL)) {
        printf("call wasm function run_demo failed. error: %s\n",
               wasm_runtime_get_exception(module_inst));
        goto fail;
    }

    printf("Wasm returned custom section handle: %d\n", results[0].of.i32);
    exit_code = results[0].of.i32 >= 0 ? 0 : 1;

fail:
    if (exec_env) {
        wasm_runtime_destroy_exec_env(exec_env);
    }
    if (module_inst) {
        wasm_runtime_deinstantiate(module_inst);
    }
    if (module) {
        wasm_runtime_unload(module);
    }
    if (buffer) {
        BH_FREE(buffer);
    }
    reset_custom_section_handles();
    wasm_runtime_destroy();
    return exit_code;
}
