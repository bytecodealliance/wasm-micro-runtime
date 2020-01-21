/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <string.h>
#include "Enclave_t.h"
#include "test_wasm.h"
#include "bh_memory.h"
#include "wasm_export.h"

static char global_heap_buf[512 * 1024] = { 0 };

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

int bh_printf(const char *message, ...);

typedef void (*bh_print_function_t)(const char* message);
extern void bh_set_print_function(bh_print_function_t pf);

void enclave_print(const char *message)
{
    ocall_print(message);
}

}

void ecall_iwasm_main()
{
    bh_set_print_function(enclave_print);

    uint8_t *wasm_file_buf = NULL;
    int wasm_file_size;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    char error_buf[128];

    if (bh_memory_init_with_pool(global_heap_buf,
                                 sizeof(global_heap_buf)) != 0) {
        ocall_print("Init global heap failed.\n");
        return;
    }

    /* initialize runtime environment */
    if (!wasm_runtime_init())
        goto fail1;

    /* load WASM byte buffer from byte buffer of include file */
    wasm_file_buf = (uint8_t*) wasm_test_file;
    wasm_file_size = sizeof(wasm_test_file);

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        ocall_print(error_buf);
        ocall_print("\n");
        goto fail2;
    }

    /* instantiate the module */
    if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module,
                                                      16 * 1024,
                                                      16 * 1024,
                                                      error_buf,
                                                      sizeof(error_buf)))) {
        ocall_print(error_buf);
        ocall_print("\n");
        goto fail3;
    }

    /* execute the main function of wasm app */
    app_instance_main(wasm_module_inst);

    /* destroy the module instance */
    wasm_runtime_deinstantiate(wasm_module_inst);

fail3:
    /* unload the module */
    wasm_runtime_unload(wasm_module);

fail2:
    /* destroy runtime environment */
    wasm_runtime_destroy();

fail1:
    bh_memory_destroy();
}

