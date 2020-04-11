/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <string.h>
#include "Enclave_t.h"
#include "test_wasm.h"
#include "wasm_export.h"

static char global_heap_buf[2* 1024 * 1024] = { 0 };

static int app_argc;
static char **app_argv;

static void*
app_instance_main(wasm_module_inst_t module_inst)
{
    const char *exception;

    wasm_application_execute_main(module_inst, app_argc, app_argv);
    if ((exception = wasm_runtime_get_exception(module_inst))) {
        ocall_print(exception);
        ocall_print("\n");
    }
    return NULL;
}

extern "C" {

typedef void (*os_print_function_t)(const char* message);
extern void os_set_print_function(os_print_function_t pf);

void enclave_print(const char *message)
{
    ocall_print(message);
}

}

void ecall_iwasm_main()
{
    os_set_print_function(enclave_print);

    uint8_t *wasm_file_buf = NULL;
    int wasm_file_size;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    RuntimeInitArgs init_args;
    char error_buf[128];

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        ocall_print("Init runtime environment failed.");
        ocall_print("\n");
        return;
    }

    /* load WASM byte buffer from byte buffer of include file */
    wasm_file_buf = (uint8_t*) wasm_test_file;
    wasm_file_size = sizeof(wasm_test_file);

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        ocall_print(error_buf);
        ocall_print("\n");
        goto fail1;
    }

    /* instantiate the module */
    if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module,
                                                      16 * 1024,
                                                      16 * 1024,
                                                      error_buf,
                                                      sizeof(error_buf)))) {
        ocall_print(error_buf);
        ocall_print("\n");
        goto fail2;
    }

    /* execute the main function of wasm app */
    app_instance_main(wasm_module_inst);

    /* destroy the module instance */
    wasm_runtime_deinstantiate(wasm_module_inst);

fail2:
    /* unload the module */
    wasm_runtime_unload(wasm_module);

fail1:
    /* destroy runtime environment */
    wasm_runtime_destroy();
}

