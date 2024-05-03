/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

// #include <autoconf.h>

#include <stdlib.h>
#include <string.h>
#include "bh_platform.h"
#include "bh_assert.h"
#include "bh_log.h"
#include "wasm_export.h"
#include "http_get.h"

// 
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>

#define CONFIG_HEAP_MEM_POOL_SIZE WASM_GLOBAL_HEAP_SIZE
#define CONFIG_APP_STACK_SIZE 8192 
#define CONFIG_APP_HEAP_SIZE 8192 * 5

static char global_heap_buf[CONFIG_HEAP_MEM_POOL_SIZE] = { 0 };

int main(void)
{
    int start, end;
    start = k_uptime_get_32();
    uint8 *wasm_file_buf = NULL;
    uint32 wasm_file_size;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    RuntimeInitArgs init_args;
    char error_buf[128];
    const char *exception;
    wasm_function_inst_t func;
    wasm_exec_env_t exec_env;
    unsigned argv[2] = { 0 };

    int log_verbose_level = 2;

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

#if WASM_ENABLE_GLOBAL_HEAP_POOL != 0
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
    printf("global heap size: %d\n", sizeof(global_heap_buf));
#else
#error "memory allocation scheme is not defined."
#endif

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        return;
    }

    bh_log_set_verbose_level(log_verbose_level);

    /* load WASM byte buffer from byte buffer of include file */
    wasm_file_buf = (uint8 *)wasm_test_file;
    wasm_file_size = sizeof(wasm_test_file);
    printf("Wasm file size: %d\n", wasm_file_size);

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        printf("Failed to load module: %s\n", error_buf);
        goto fail1;
    }

    /* Set the WASI context */
#if WASM_ENABLE_LIBC_WASI != 0
#define ADDRESS_POOL_SIZE 1
    const char *addr_pool[ADDRESS_POOL_SIZE] = {
        "192.0.2.10/24",
        // Add more addresses here if needed
    };
    /* No dir list => No file system 
     * dir_cont = 0
     * No mapped dir list => No file system 
     * map_dir_cont = 0
     * No environment variables
     * env_count = 0
     * No command line arguments
     * argv  0 
     */
    wasm_runtime_set_wasi_args(wasm_module, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
    wasm_runtime_set_wasi_addr_pool(wasm_module, addr_pool, ADDRESS_POOL_SIZE);
    wasm_runtime_set_wasi_ns_lookup_pool(wasm_module, NULL, 0);
#endif

    /* instantiate the module */
    if (!(wasm_module_inst = wasm_runtime_instantiate(
              wasm_module, CONFIG_APP_STACK_SIZE, CONFIG_APP_HEAP_SIZE,
              error_buf, sizeof(error_buf)))) {
        printf("Failed to instantiate module: %s\n", error_buf);
        goto fail2;
    }

    /* invoke the main function */
    // if ((func = wasm_runtime_lookup_function(wasm_module_inst, "_start"))) {
    if ((func = wasm_runtime_lookup_function(wasm_module_inst, "__main_argc_argv"))) {
        printf("Creating exec_env\n");
        exec_env = wasm_runtime_create_exec_env(wasm_module_inst, CONFIG_APP_HEAP_SIZE);
        if (!exec_env) {
            printf("Create exec env failed\n");
            return -1;
        }

        printf("Calling main function\n");
        bool ret = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        
        if(!ret) {
            printf("Failed to call main function\n");
            goto fail3;
        }

        wasm_runtime_destroy_exec_env(exec_env);
    }
    else {
        printf("Failed to lookup function main\n");
        return -1;
    }

fail3:
    printf("%s\n", wasm_runtime_get_exception(wasm_module_inst));

    int rc = wasm_runtime_get_wasi_exit_code(wasm_module_inst);
    printf("wasi exit code: %d\n", rc); // 1 = _WASI_E2BIG

    /* destroy the module instance */
    wasm_runtime_deinstantiate(wasm_module_inst);

fail2:
    /* unload the module */
    wasm_runtime_unload(wasm_module);

fail1:
    /* destroy runtime environment */
    wasm_runtime_destroy();

    end = k_uptime_get_32();

    printf("elapsed: %d\n", (end - start));

    return 0;
}