/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <string.h>

#include "bh_platform.h"
#include "bh_read_file.h"
#include "wasm_export.h"

static int app_argc;
static char **app_argv;

#define MODULE_PATH ("--module-path=")

static int
print_help()
{
    /* clang-format off */
    printf("Usage: iwasm [-options] wasm_file [args...]\n");
    printf("options:\n");
    printf("  -f|--function name     Specify a function name of the module to run rather\n"
           "                         than main\n");
#if WASM_ENABLE_LOG != 0
    printf("  -v=n                   Set log verbose level (0 to 5, default is 2) larger\n"
           "                         level with more log\n");
#endif
    printf("  --stack-size=n         Set maximum stack size in bytes, default is 16 KB\n");
    printf("  --heap-size=n          Set maximum heap size in bytes, default is 16 KB\n");
#if WASM_ENABLE_LIBC_WASI != 0
    printf("  --env=<env>            Pass wasi environment variables with \"key=value\"\n");
    printf("                         to the program, for example:\n");
    printf("                           --env=\"key1=value1\" --env=\"key2=value2\"\n");
    printf("  --dir=<dir>            Grant wasi access to the given host directories\n");
    printf("                         to the program, for example:\n");
    printf("                           --dir=<dir1> --dir=<dir2>\n");
#endif
#if WASM_ENABLE_MULTI_MODULE != 0
    printf("  --module-path=         Indicate a module search path. default is current\n"
           "                         directory('./')\n");
#endif
#if WASM_ENABLE_LIB_PTHREAD != 0
    printf("  --max-threads=n        Set maximum thread number per cluster, default is 4\n");
#endif
    /* clang-format on */
    return 1;
}

static void *
app_instance_main(wasm_module_inst_t module_inst)
{
    const char *exception;

    wasm_application_execute_main(module_inst, app_argc, app_argv);
    if ((exception = wasm_runtime_get_exception(module_inst)))
        printf("%s\n", exception);
    return NULL;
}

static void *
app_instance_func(wasm_module_inst_t module_inst, const char *func_name)
{
    wasm_application_execute_func(module_inst, func_name, app_argc - 1,
                                  app_argv + 1);
    /* The result of wasm function or exception info was output inside
       wasm_application_execute_func(), here we don't output them again. */
    return NULL;
}

#if WASM_ENABLE_LIBC_WASI != 0
static bool
validate_env_str(char *env)
{
    char *p = env;
    int key_len = 0;

    while (*p != '\0' && *p != '=') {
        key_len++;
        p++;
    }

    if (*p != '=' || key_len == 0)
        return false;

    return true;
}
#endif

#if WASM_ENABLE_GLOBAL_HEAP_POOL != 0
static char global_heap_buf[WASM_GLOBAL_HEAP_SIZE * BH_KB] = { 0 };
#endif

#if WASM_ENABLE_MULTI_MODULE != 0
static char *
handle_module_path(const char *module_path)
{
    /* next character after = */
    return (strchr(module_path, '=')) + 1;
}

static char *module_search_path = ".";
static bool
module_reader_callback(const char *module_name,
                       uint8 **p_buffer,
                       uint32 *p_size)
{
    const char *format = "%s/%s.wasm";
    int sz = strlen(module_search_path) + strlen("/") + strlen(module_name)
             + strlen(".wasm") + 1;
    char *wasm_file_name = BH_MALLOC(sz);
    if (!wasm_file_name) {
        return false;
    }

    snprintf(wasm_file_name, sz, format, module_search_path, module_name);

    *p_buffer = (uint8_t *)bh_read_file_to_buffer(wasm_file_name, p_size);

    wasm_runtime_free(wasm_file_name);
    return *p_buffer != NULL;
}

static void
moudle_destroyer(uint8 *buffer, uint32 size)
{
    if (!buffer) {
        return;
    }

    wasm_runtime_free(buffer);
    buffer = NULL;
}
#endif /* WASM_ENABLE_MULTI_MODULE */

int
main(int argc, char *argv[])
{
    char *wasm_file = NULL;
    const char *func_name = NULL;
    uint8 *wasm_file_buf = NULL;
    uint32 wasm_file_size;
    uint32 stack_size = 16 * 1024, heap_size = 16 * 1024;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    RuntimeInitArgs init_args;
    char error_buf[128] = { 0 };
#if WASM_ENABLE_LOG != 0
    int log_verbose_level = 2;
#endif
#if WASM_ENABLE_LIBC_WASI != 0
    const char *dir_list[8] = { NULL };
    uint32 dir_list_size = 0;
    const char *env_list[8] = { NULL };
    uint32 env_list_size = 0;
#endif

    /* Process options.  */
    for (argc--, argv++; argc > 0 && argv[0][0] == '-'; argc--, argv++) {
        if (!strcmp(argv[0], "-f") || !strcmp(argv[0], "--function")) {
            argc--, argv++;
            if (argc < 2) {
                print_help();
                return 0;
            }
            func_name = argv[0];
        }
#if WASM_ENABLE_LOG != 0
        else if (!strncmp(argv[0], "-v=", 3)) {
            log_verbose_level = atoi(argv[0] + 3);
            if (log_verbose_level < 0 || log_verbose_level > 5)
                return print_help();
        }
#endif
        else if (!strncmp(argv[0], "--stack-size=", 13)) {
            if (argv[0][13] == '\0')
                return print_help();
            stack_size = atoi(argv[0] + 13);
        }
        else if (!strncmp(argv[0], "--heap-size=", 12)) {
            if (argv[0][12] == '\0')
                return print_help();
            heap_size = atoi(argv[0] + 12);
        }
#if WASM_ENABLE_LIBC_WASI != 0
        else if (!strncmp(argv[0], "--dir=", 6)) {
            if (argv[0][6] == '\0')
                return print_help();
            if (dir_list_size >= sizeof(dir_list) / sizeof(char *)) {
                printf("Only allow max dir number %d\n",
                       (int)(sizeof(dir_list) / sizeof(char *)));
                return -1;
            }
            dir_list[dir_list_size++] = argv[0] + 6;
        }
        else if (!strncmp(argv[0], "--env=", 6)) {
            char *tmp_env;

            if (argv[0][6] == '\0')
                return print_help();
            if (env_list_size >= sizeof(env_list) / sizeof(char *)) {
                printf("Only allow max env number %d\n",
                       (int)(sizeof(env_list) / sizeof(char *)));
                return -1;
            }
            tmp_env = argv[0] + 6;
            if (validate_env_str(tmp_env))
                env_list[env_list_size++] = tmp_env;
            else {
                printf("Wasm parse env string failed: expect \"key=value\", "
                       "got \"%s\"\n",
                       tmp_env);
                return print_help();
            }
        }
#endif /* WASM_ENABLE_LIBC_WASI */
#if WASM_ENABLE_MULTI_MODULE != 0
        else if (!strncmp(argv[0], MODULE_PATH, strlen(MODULE_PATH))) {
            module_search_path = handle_module_path(argv[0]);
            if (!strlen(module_search_path)) {
                return print_help();
            }
        }
#endif
#if WASM_ENABLE_LIB_PTHREAD != 0
        else if (!strncmp(argv[0], "--max-threads=", 14)) {
            if (argv[0][14] == '\0')
                return print_help();
            wasm_runtime_set_max_thread_num(atoi(argv[0] + 14));
        }
#endif
        else
            return print_help();
    }

    if (argc == 0)
        return print_help();

    wasm_file = argv[0];
    app_argc = argc;
    app_argv = argv;

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

#if WASM_ENABLE_GLOBAL_HEAP_POOL != 0
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
#else
    init_args.mem_alloc_type = Alloc_With_Allocator;
    init_args.mem_alloc_option.allocator.malloc_func = malloc;
    init_args.mem_alloc_option.allocator.realloc_func = realloc;
    init_args.mem_alloc_option.allocator.free_func = free;
#endif

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

#if WASM_ENABLE_MULTI_MODULE != 0
    wasm_runtime_set_module_reader(module_reader_callback, moudle_destroyer);
#endif

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail2;
    }

#if WASM_ENABLE_LIBC_WASI != 0
    wasm_runtime_set_wasi_args(wasm_module, dir_list, dir_list_size, NULL, 0,
                               env_list, env_list_size, argv, argc);
#endif

    /* instantiate the module */
    if (!(wasm_module_inst =
            wasm_runtime_instantiate(wasm_module, stack_size, heap_size,
                                     error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail3;
    }

    if (func_name)
        app_instance_func(wasm_module_inst, func_name);
    else
        app_instance_main(wasm_module_inst);

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
