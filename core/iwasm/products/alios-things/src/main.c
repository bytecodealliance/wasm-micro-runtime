/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include "bh_platform.h"
#include "wasm_assert.h"
#include "wasm_log.h"
#include "wasm_platform_log.h"
#include "wasm_thread.h"
#include "wasm_export.h"
#include "wasm_memory.h"
#include "bh_memory.h"
#include "test_wasm.h"

static int app_argc;
static char **app_argv;

static void*
app_instance_main(wasm_module_inst_t module_inst)
{
    const char *exception;

    wasm_application_execute_main(module_inst, app_argc, app_argv);
    if ((exception = wasm_runtime_get_exception(module_inst)))
        wasm_printf("%s\n", exception);
    return NULL;
}

static char global_heap_buf[256 * 1024] = { 0 };

void iwasm_main(void *arg1)
{
    uint8 *wasm_file_buf = NULL;
    int wasm_file_size;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    char error_buf[128];
#if WASM_ENABLE_LOG != 0
    int log_verbose_level = 1;
#endif

    (void) arg1;

    if (bh_memory_init_with_pool(global_heap_buf, sizeof(global_heap_buf))
            != 0) {
        wasm_printf("Init global heap failed.\n");
        return;
    }

    /* initialize runtime environment */
    if (!wasm_runtime_init())
        goto fail1;

#if WASM_ENABLE_LOG != 0
    wasm_log_set_verbose_level(log_verbose_level);
#endif

    /* load WASM byte buffer from byte buffer of include file */
    wasm_file_buf = (uint8*) wasm_test_file;
    wasm_file_size = sizeof(wasm_test_file);

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
            error_buf, sizeof(error_buf)))) {
        wasm_printf("%s\n", error_buf);
        goto fail2;
    }

    /* instantiate the module */
    if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module, 8 * 1024,
            8 * 1024, error_buf, sizeof(error_buf)))) {
        wasm_printf("%s\n", error_buf);
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

#define DEFAULT_THREAD_STACKSIZE (6 * 1024)
#define DEFAULT_THREAD_PRIORITY 50

bool iwasm_init(void)
{
    int ret = aos_task_new("wasm-main", iwasm_main, NULL,
    DEFAULT_THREAD_STACKSIZE);
    return ret == 0 ? true : false;
}

