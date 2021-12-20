/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wasm_export.h"
#include "bh_platform.h"
#include "test_wasm.h"

static void *
app_instance_main(wasm_module_inst_t module_inst)
{
    const char *exception;

    wasm_application_execute_main(module_inst, 0, NULL);
    if ((exception = wasm_runtime_get_exception(module_inst)))
        printf("%s\n", exception);
    return NULL;
}

void *
iwasm_main(void *arg)
{
    (void)arg; /* unused */
    /* setup variables for instantiating and running the wasm module */
    uint8_t *wasm_file_buf = NULL;
    unsigned wasm_file_buf_size = 0;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    char error_buf[128];
    void *ret;
    RuntimeInitArgs init_args;

    /* configure memory allocation */
    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_Allocator;
    init_args.mem_alloc_option.allocator.malloc_func = (void *)os_malloc;
    init_args.mem_alloc_option.allocator.realloc_func = (void *)os_realloc;
    init_args.mem_alloc_option.allocator.free_func = (void *)os_free;

    printf("wasm_runtime_full_init\n");
    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime failed.\n");
        return NULL;
    }

    /* load WASM byte buffer from byte buffer of include file */
    printf("use an internal test file, that's going to output Hello World\n");
    wasm_file_buf = (uint8_t *)wasm_test_file;
    wasm_file_buf_size = sizeof(wasm_test_file);

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_buf_size,
                                          error_buf, sizeof(error_buf)))) {
        printf("Error in wasm_runtime_load: %s\n", error_buf);
        goto fail1;
    }

    printf("about to call wasm_runtime_instantiate\n");
    if (!(wasm_module_inst =
              wasm_runtime_instantiate(wasm_module, 32 * 1024, // stack size
                                       32 * 1024,              // heap size
                                       error_buf, sizeof(error_buf)))) {
        printf("Error while instantiating: %s\n", error_buf);
        goto fail2;
    }

    printf("run main() of the application\n");
    ret = app_instance_main(wasm_module_inst);
    assert(!ret);

    /* destroy the module instance */
    printf("wasm_runtime_deinstantiate\n");
    wasm_runtime_deinstantiate(wasm_module_inst);

fail2:
    /* unload the module */
    printf("wasm_runtime_unload\n");
    wasm_runtime_unload(wasm_module);

fail1:
    /* destroy runtime environment */
    printf("wasm_runtime_destroy\n");
    wasm_runtime_destroy();

    return NULL;
}

void
app_main(void)
{
    pthread_t t;
    int res;

    res = pthread_create(&t, NULL, iwasm_main, (void *)NULL);
    assert(res == 0);

    res = pthread_join(t, NULL);
    assert(res == 0);

    printf("Exiting... \n");
}
