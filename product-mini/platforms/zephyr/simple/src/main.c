/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdlib.h>
#include <string.h>
#include "bh_platform.h"
#include "bh_assert.h"
#include "bh_log.h"
#include "wasm_export.h"
#include "test_wasm.h"

#include <zephyr.h>
#include <sys/printk.h>

#define CONFIG_GLOBAL_HEAP_BUF_SIZE 131072
#define CONFIG_APP_STACK_SIZE 8192
#define CONFIG_APP_HEAP_SIZE 8192

#ifdef CONFIG_NO_OPTIMIZATIONS
#define CONFIG_MAIN_THREAD_STACK_SIZE 8192
#else
#define CONFIG_MAIN_THREAD_STACK_SIZE 4096
#endif

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
        printf("%s\n", exception);
    return NULL;
}

static char global_heap_buf[CONFIG_GLOBAL_HEAP_BUF_SIZE] = { 0 };

#ifdef CONFIG_BOARD_ESP32
#include "mem_alloc.h"
/*
esp32_technical_reference_manual:
"
The capacity of Internal SRAM 1 is 128 KB. Either CPU can read and write this memory at addresses
0x3FFE_0000 ~ 0x3FFF_FFFF of the data bus, and also at addresses 0x400A_0000 ~ 0x400B_FFFF of the
instruction bus.
"

The custom linker script defines dram0_1_seg and map it to 0x400A_0000 ~ 0x400B_FFFF for instruction bus access.
Here we define the buffer that will be placed to dram0_1_seg.
*/
static char esp32_executable_memory_buf[100 * 1024] __attribute__((section (".aot_code_buf"))) = { 0 };

/* the poll allocator for executable memory */
static mem_allocator_t esp32_exec_mem_pool_allocator;

static int
esp32_exec_mem_init()
{
    if (!(esp32_exec_mem_pool_allocator =
                mem_allocator_create(esp32_executable_memory_buf,
                                     sizeof(esp32_executable_memory_buf))))
        return -1;

    return 0;
}

static void
esp32_exec_mem_destroy()
{
    mem_allocator_destroy(esp32_exec_mem_pool_allocator);
}

static void *
esp32_exec_mem_alloc(unsigned int size)
{
    return mem_allocator_malloc(esp32_exec_mem_pool_allocator, size);
}

static void
esp32_exec_mem_free(void *addr)
{
    mem_allocator_free(esp32_exec_mem_pool_allocator, addr);
}
#endif /* end of #ifdef CONFIG_BOARD_ESP32 */

void iwasm_main(void *arg1, void *arg2, void *arg3)
{
    int start, end;
    start = k_uptime_get_32();
    uint8 *wasm_file_buf = NULL;
    uint32 wasm_file_size;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    RuntimeInitArgs init_args;
    char error_buf[128];
#if WASM_ENABLE_LOG != 0
    int log_verbose_level = 2;
#endif

    (void) arg1;
    (void) arg2;
    (void) arg3;


    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        return;
    }

#ifdef CONFIG_BOARD_ESP32
    /* Initialize executable memory */
    if (esp32_exec_mem_init() != 0) {
        printf("Init executable memory failed.\n");
        goto fail1;
    }
    /* Set hook functions for executable memory management */
    set_exec_mem_alloc_func(esp32_exec_mem_alloc, esp32_exec_mem_free);
#endif

#if WASM_ENABLE_LOG != 0
    bh_log_set_verbose_level(log_verbose_level);
#endif

    /* load WASM byte buffer from byte buffer of include file */
    wasm_file_buf = (uint8*) wasm_test_file;
    wasm_file_size = sizeof(wasm_test_file);

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
#ifdef CONFIG_BOARD_ESP32
        goto fail1_1;
#else
        goto fail1;
#endif
    }

    /* instantiate the module */
    if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module,
                                                      CONFIG_APP_STACK_SIZE,
                                                      CONFIG_APP_HEAP_SIZE,
                                                      error_buf,
                                                      sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail2;
    }

    /* invoke the main function */
    app_instance_main(wasm_module_inst);

    /* destroy the module instance */
    wasm_runtime_deinstantiate(wasm_module_inst);

fail2:
    /* unload the module */
    wasm_runtime_unload(wasm_module);

#ifdef CONFIG_BOARD_ESP32
fail1_1:
    /* destroy executable memory */
    esp32_exec_mem_destroy();
#endif

fail1:
    /* destroy runtime environment */
    wasm_runtime_destroy();

    end = k_uptime_get_32();

    printf("elpase: %d\n", (end - start));
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

