/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "wasm_export.h"
#include "test_wasm.h"

#define CONFIG_GLOBAL_HEAP_BUF_SIZE 131072
#define CONFIG_APP_STACK_SIZE 8192
#define CONFIG_APP_HEAP_SIZE 8192

static int app_argc;
static char **app_argv;

static void *
app_instance_main(wasm_module_inst_t module_inst)
{
    const char *exception;

    wasm_application_execute_main(module_inst, app_argc, app_argv);
    if ((exception = wasm_runtime_get_exception(module_inst)))
        os_printf("%s\n", exception);
    return NULL;
}

static char global_heap_buf[CONFIG_GLOBAL_HEAP_BUF_SIZE] = { 0 };

void
iwasm_main(void)
{
    uint8 *wasm_file_buf = NULL;
    uint32 wasm_file_size;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    RuntimeInitArgs init_args;
    char error_buf[128];
#if WASM_ENABLE_LOG != 0
    int log_verbose_level = 2;
#endif

    os_printf("### iwasm main begin\n");

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        os_printf("Init runtime environment failed.\n");
        return;
    }

    os_printf("### wasm runtime initialized.\n");

#if WASM_ENABLE_LOG != 0
    bh_log_set_verbose_level(log_verbose_level);
#endif

    /* load WASM byte buffer from byte buffer of include file */
    wasm_file_buf = (uint8 *)wasm_test_file;
    wasm_file_size = sizeof(wasm_test_file);

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        os_printf("%s\n", error_buf);
        goto fail1;
    }

    os_printf("### wasm runtime load module success.\n");

    /* instantiate the module */
    if (!(wasm_module_inst = wasm_runtime_instantiate(
              wasm_module, CONFIG_APP_STACK_SIZE, CONFIG_APP_HEAP_SIZE,
              error_buf, sizeof(error_buf)))) {
        os_printf("%s\n", error_buf);
        goto fail2;
    }

    os_printf("### wasm runtime instantiate module success.\n");

    /* invoke the main function */
    app_instance_main(wasm_module_inst);

    os_printf("### wasm runtime execute app's main function success.\n");

    /* destroy the module instance */
    wasm_runtime_deinstantiate(wasm_module_inst);

    os_printf("### wasm runtime deinstantiate module success.\n");

fail2:
    /* unload the module */
    wasm_runtime_unload(wasm_module);

    os_printf("### wasm runtime unload module success.\n");
fail1:
    /* destroy runtime environment */
    wasm_runtime_destroy();
    os_printf("### wasm runtime destroy success.\n");
}
