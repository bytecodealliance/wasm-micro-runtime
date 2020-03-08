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
#include "bh_assert.h"
#include "bh_log.h"
#include "bh_memory.h"
#include "wasm_export.h"

static int app_argc;
static char **app_argv;

static int print_help()
{
    bh_printf("Usage: iwasm [-options] wasm_file [args...]\n");
    bh_printf("options:\n");
    bh_printf("  -f|--function name     Specify function name to run in module\n"
              "                         rather than main\n");
#if WASM_ENABLE_LOG != 0
    bh_printf("  -v=X                   Set log verbose level (0 to 5, default is 2),\n"
              "                         larger level with more log\n");
#endif
    bh_printf("  --repl                 Start a very simple REPL (read-eval-print-loop) mode\n"
              "                         that runs commands in the form of `FUNC ARG...`\n");
#if WASM_ENABLE_LIBC_WASI != 0
    bh_printf("  --env=<env>            Pass wasi environment variables with \"key=value\"\n");
    bh_printf("                         to the program, for example:\n");
    bh_printf("                           --env=\"key1=value1\" --env=\"key2=value2\"\n");
    bh_printf("  --dir=<dir>            Grant wasi access to the given host directories\n");
    bh_printf("                         to the program, for example:\n");
    bh_printf("                           --dir=<dir1> --dir=<dir2>\n");
#endif

    return 1;
}

static void*
app_instance_main(wasm_module_inst_t module_inst)
{
    const char *exception;

    wasm_application_execute_main(module_inst, app_argc, app_argv);
    if ((exception = wasm_runtime_get_exception(module_inst)))
        bh_printf("%s\n", exception);
    return NULL;
}

static void*
app_instance_func(wasm_module_inst_t module_inst, const char *func_name)
{
    wasm_application_execute_func(module_inst, func_name, app_argc - 1,
                                  app_argv + 1);
    /* The result of wasm function or exception info was output inside
       wasm_application_execute_func(), here we don't output them again. */
    return NULL;
}

/**
 * Split a space separated strings into an array of strings
 * Returns NULL on failure
 * Memory must be freed by caller
 * Based on: http://stackoverflow.com/a/11198630/471795
 */
static char **
split_string(char *str, int *count)
{
    char **res = NULL;
    char *p;
    int idx = 0;

    /* split string and append tokens to 'res' */
    do {
        p = strtok(str, " ");
        str = NULL;
        res = (char**) realloc(res, sizeof(char*) * (uint32)(idx + 1));
        if (res == NULL) {
            return NULL;
        }
        res[idx++] = p;
    } while (p);

    if (count) {
        *count = idx - 1;
    }
    return res;
}

static void*
app_instance_repl(wasm_module_inst_t module_inst)
{
    char *cmd = NULL;
    size_t len = 0;
    ssize_t n;

    while ((bh_printf("webassembly> "), n = getline(&cmd, &len, stdin)) != -1) {
        bh_assert(n > 0);
        if (cmd[n - 1] == '\n') {
            if (n == 1)
                continue;
            else
                cmd[n - 1] = '\0';
        }
        app_argv = split_string(cmd, &app_argc);
        if (app_argv == NULL) {
            LOG_ERROR("Wasm prepare param failed: split string failed.\n");
            break;
        }
        if (app_argc != 0) {
            wasm_application_execute_func(module_inst, app_argv[0],
                                          app_argc - 1, app_argv + 1);
        }
        free(app_argv);
    }
    free(cmd);
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

#define USE_GLOBAL_HEAP_BUF 0

#if USE_GLOBAL_HEAP_BUF != 0
static char global_heap_buf[10 * 1024 * 1024] = { 0 };
#endif

int main(int argc, char *argv[])
{
    char *wasm_file = NULL;
    const char *func_name = NULL;
    uint8 *wasm_file_buf = NULL;
    uint32 wasm_file_size;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    RuntimeInitArgs init_args;
    char error_buf[128] = { 0 };
#if WASM_ENABLE_LOG != 0
    int log_verbose_level = 2;
#endif
    bool is_repl_mode = false;
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
        else if (!strcmp(argv[0], "--repl"))
            is_repl_mode = true;
#if WASM_ENABLE_LIBC_WASI != 0
        else if (!strncmp(argv[0], "--dir=", 6)) {
            if (argv[0][6] == '\0')
                return print_help();
            if (dir_list_size >= sizeof(dir_list) / sizeof(char*)) {
                bh_printf("Only allow max dir number %d\n",
                          (int)(sizeof(dir_list) / sizeof(char*)));
                return -1;
            }
            dir_list[dir_list_size++] = argv[0] + 6;
        }
        else if (!strncmp(argv[0], "--env=", 6)) {
            char *tmp_env;

            if (argv[0][6] == '\0')
                return print_help();
            if (env_list_size >= sizeof(env_list) / sizeof(char*)) {
                bh_printf("Only allow max env number %d\n",
                          (int)(sizeof(env_list) / sizeof(char*)));
                return -1;
            }
            tmp_env = argv[0] + 6;
            if (validate_env_str(tmp_env))
                env_list[env_list_size++] = tmp_env;
            else {
                bh_printf("Wasm parse env string failed: expect \"key=value\", got \"%s\"\n",
                          tmp_env);
                return print_help();
            }
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

#if USE_GLOBAL_HEAP_BUF != 0
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc.pool.heap_size = sizeof(global_heap_buf);
#else
    init_args.mem_alloc_type = Alloc_With_Allocator;
    init_args.mem_alloc.allocator.malloc_func = malloc;
    init_args.mem_alloc.allocator.realloc_func = realloc;
    init_args.mem_alloc.allocator.free_func = free;
#endif

    if (!wasm_runtime_full_init(&init_args)) {
        bh_printf("Init runtime environment failed.\n");
        return -1;
    }

    bh_log_set_verbose_level(log_verbose_level);

    /* load WASM byte buffer from WASM bin file */
    if (!(wasm_file_buf = (uint8*) bh_read_file_to_buffer(wasm_file,
                                                          &wasm_file_size)))
        goto fail1;

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        bh_printf("%s\n", error_buf);
        goto fail2;
    }

#if WASM_ENABLE_LIBC_WASI != 0
    wasm_runtime_set_wasi_args(wasm_module,
                               dir_list, dir_list_size,
                               NULL, 0,
                               env_list, env_list_size,
                               argv, argc);
#endif

    /* instantiate the module */
    if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module,
                                                      48 * 1024, /* stack size */
                                                      16 * 1024, /* heap size */
                                                      error_buf,
                                                      sizeof(error_buf)))) {
        bh_printf("%s\n", error_buf);
        goto fail3;
    }

    if (is_repl_mode)
        app_instance_repl(wasm_module_inst);
    else if (func_name)
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
    bh_free(wasm_file_buf);

fail1:
    /* destroy runtime environment */
    wasm_runtime_full_destroy();
    return 0;
}

