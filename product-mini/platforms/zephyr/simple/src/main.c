/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdlib.h>
#include <string.h>
#include "bh_platform.h"
#include "bh_assert.h"
#include "bh_log.h"
#include "bh_memory.h"
#include "wasm_export.h"
#include "test_wasm.h"

#define CONFIG_GLOBAL_HEAP_BUF_SIZE 131072
#define CONFIG_APP_STACK_SIZE 8192
#define CONFIG_APP_HEAP_SIZE 8192
#define CONFIG_MAIN_THREAD_STACK_SIZE 4096

static int app_argc;
static char **app_argv;

/**
 * Find the unique main function from a WASM module instance
 * and execute that function.
 *
 * @param module_inst the WASM module instance
 * @param argc the number of arguments
 * @param argv the arguments array
 *
 * @return true if the main function is called, false otherwise.
 */
bool
wasm_application_execute_main(wasm_module_inst_t module_inst,
                              int argc, char *argv[]);

static void*
app_instance_main(wasm_module_inst_t module_inst)
{
    const char *exception;

    wasm_application_execute_main(module_inst, app_argc, app_argv);
    if ((exception = wasm_runtime_get_exception(module_inst)))
        bh_printf("%s\n", exception);
    return NULL;
}

static char global_heap_buf[CONFIG_GLOBAL_HEAP_BUF_SIZE] = { 0 };

void iwasm_main(void *arg1, void *arg2, void *arg3)
{
    uint8 *wasm_file_buf = NULL;
    uint32 wasm_file_size;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    char error_buf[128];
#if WASM_ENABLE_LOG != 0
    int log_verbose_level = 2;
#endif

    (void) arg1;
    (void) arg2;
    (void) arg3;

    if (bh_memory_init_with_pool(global_heap_buf, sizeof(global_heap_buf))
        != 0) {
        bh_printf("Init global heap failed.\n");
        return;
    }

    /* initialize runtime environment */
    if (!wasm_runtime_init())
        goto fail1;

#if WASM_ENABLE_LOG != 0
    bh_log_set_verbose_level(log_verbose_level);
#endif

    /* load WASM byte buffer from byte buffer of include file */
    wasm_file_buf = (uint8*) wasm_test_file;
    wasm_file_size = sizeof(wasm_test_file);

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        bh_printf("%s\n", error_buf);
        goto fail2;
    }

    /* instantiate the module */
    if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module,
                                                      CONFIG_APP_STACK_SIZE,
                                                      CONFIG_APP_HEAP_SIZE,
                                                      error_buf,
                                                      sizeof(error_buf)))) {
        bh_printf("%s\n", error_buf);
        goto fail3;
    }

    app_instance_main(wasm_module_inst);

    /* destroy the module instance */
    wasm_runtime_deinstantiate(wasm_module_inst);

    fail3:
    /* unload the module */
    wasm_runtime_unload(wasm_module);

    fail2:
    /* destroy runtime environment */
    wasm_runtime_destroy();

    fail1: bh_memory_destroy();
}

#define MAIN_THREAD_STACK_SIZE (CONFIG_MAIN_THREAD_STACK_SIZE)
#define MAIN_THREAD_PRIORITY 5

K_THREAD_STACK_DEFINE(iwasm_main_thread_stack, MAIN_THREAD_STACK_SIZE);
static struct k_thread iwasm_main_thread;

bool iwasm_init(void)
{
    k_tid_t tid = k_thread_create(&iwasm_main_thread, iwasm_main_thread_stack,
                                  MAIN_THREAD_STACK_SIZE,
                                  iwasm_main, NULL, NULL, NULL,
                                  MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
    return tid ? true : false;
}

void main(void)
{
    iwasm_init();
}

