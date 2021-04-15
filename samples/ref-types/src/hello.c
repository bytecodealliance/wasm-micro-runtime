/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "bh_read_file.h"
#include "wasm_export.h"

#define USE_GLOBAL_HEAP_BUF 0

#if USE_GLOBAL_HEAP_BUF != 0
static char global_heap_buf[10 * 1024 * 1024] = { 0 };
#endif

static int
test_write_wrapper(wasm_exec_env_t exec_env,
                   uint32 externref_idx_of_file,
                   const char *str, int len)
{
    FILE *file;
    char buf[16];

    printf("## retrieve file handle from externref index\n");
    if (!wasm_externref_ref2obj(externref_idx_of_file, (void **)&file)) {
        printf("failed to get host object from externref index!\n");
        return -1;
    }

    snprintf(buf, sizeof(buf), "%%%ds", len);

    printf("## write string to file: ");
    printf(buf, str);

    return fprintf(file, buf, str);
}

static NativeSymbol native_symbols[] = {
    { "test_write", test_write_wrapper, "(i*~)i", NULL }
};

int
main(int argc, char *argv[])
{
    char *wasm_file = "hello.wasm";
    uint8 *wasm_file_buf = NULL;
    uint32 wasm_file_size, externref_idx;
    uint32 stack_size = 16 * 1024, heap_size = 16 * 1024;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    wasm_function_inst_t func_inst = NULL;
    wasm_exec_env_t exec_env = NULL;
    RuntimeInitArgs init_args;
    char error_buf[128] = { 0 };
    const char *exce;
    unsigned argv1[8];
#if WASM_ENABLE_LOG != 0
    int log_verbose_level = 2;
#endif
    FILE *file;

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

#if USE_GLOBAL_HEAP_BUF != 0
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
#else
    init_args.mem_alloc_type = Alloc_With_Allocator;
    init_args.mem_alloc_option.allocator.malloc_func = malloc;
    init_args.mem_alloc_option.allocator.realloc_func = realloc;
    init_args.mem_alloc_option.allocator.free_func = free;
#endif

    init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
    init_args.native_module_name = "env";
    init_args.native_symbols = native_symbols;

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        return -1;
    }

#if WASM_ENABLE_LOG != 0
    bh_log_set_verbose_level(log_verbose_level);
#endif

    /* load WASM byte buffer from WASM bin file */
    if (!(wasm_file_buf =
            (uint8 *)bh_read_file_to_buffer(wasm_file, &wasm_file_size)))
        goto fail1;

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail2;
    }

    /* instantiate the module */
    if (!(wasm_module_inst =
            wasm_runtime_instantiate(wasm_module, stack_size, heap_size,
                                     error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail3;
    }

    /* lookup function instance */
    if (!(func_inst = wasm_runtime_lookup_function(wasm_module_inst,
                                                   "test", NULL))) {
        printf("%s\n", "lookup function test failed");
        goto fail4;
    }

    if (!(exec_env =
            wasm_runtime_create_exec_env(wasm_module_inst, stack_size))) {
        printf("%s\n", "create exec env failed");
        goto fail4;
    }

    printf("## open file test.txt\n");
    if (!(file = fopen("test.txt", "wb+"))) {
        printf("%s\n", "open file text.txt failed");
        goto fail5;
    }

    printf("## map file handle to externref index\n");
    if (!wasm_externref_obj2ref(wasm_module_inst, file, &externref_idx)) {
        printf("%s\n", "map host object to externref index failed");
        goto fail6;
    }

    printf("## call wasm function with externref index\n");
    argv1[0] = externref_idx;
    wasm_runtime_call_wasm(exec_env, func_inst, 1, argv1);

    if ((exce = wasm_runtime_get_exception(wasm_module_inst))) {
        printf("Exception: %s\n", exce);
    }

fail6:
    fclose(file);

fail5:
    /* destroy exec env */
    wasm_runtime_destroy_exec_env(exec_env);

fail4:
    /* destroy the module instance */
    wasm_runtime_deinstantiate(wasm_module_inst);

fail3:
    /* unload the module */
    wasm_runtime_unload(wasm_module);

fail2:
    /* free the file buffer */
    wasm_runtime_free(wasm_file_buf);

fail1:
    /* destroy runtime environment */
    wasm_runtime_destroy();
    return 0;
}
