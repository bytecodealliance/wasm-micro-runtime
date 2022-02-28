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

/* clang-format off */
#if 0
#if AS_HARDCODE_ABORT != 0
static void as_abort(int msg, int file, int line, int column)
{
    fprintf(stdout, "wamr: as_abort\r\n");
    exit(1);
}
#endif
#endif

static int
print_help()
{
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
    printf("  --repl                 Start a very simple REPL (read-eval-print-loop) mode\n"
           "                         that runs commands in the form of \"FUNC ARG...\"\n");
    printf("  --xip                  Enable XIP (Execution In Place) mode to run AOT file\n"
           "                         generated with \"--enable-indirect-mode\" flag\n");
#if WASM_ENABLE_DYNAMIC_LINKING != 0
    printf("  --enable-dlopen=n      Enable explictily dynamic module loading\n"
           "                         n is a 5-bit bitmap, each bit indicates a feature\n"
           "                         from bits[0] to bits[4], they are:\n"
           "                         bind mode, currently always lazy binding\n"
           "                         where memory allocator comes from, 0 - from builtin libc; 1 - from root module\n"
           "                         if use table space to store module exports function, 0 - no; 1 - yes\n"
           "                         if root module is a AS module, 0 - no; 1 - yes\n"
           "                         if enable cache to save symbol resolve result, 0 - no; 1 - yes, currently not supported yet\n"
           "                         e.g. n = 14 (0b1110), indicates memory from root module, lazy binding, root module is AS module and use table space\n");
    printf("  --disable-auto-ext     Disable automatically update ext name in AOT mode\n");
#endif
#if WASM_ENABLE_LIBC_WASI != 0
    printf("  --env=<env>            Pass wasi environment variables with \"key=value\"\n");
    printf("                         to the program, for example:\n");
    printf("                           --env=\"key1=value1\" --env=\"key2=value2\"\n");
    printf("  --dir=<dir>            Grant wasi access to the given host directories\n");
    printf("                         to the program, for example:\n");
    printf("                           --dir=<dir1> --dir=<dir2>\n");
#endif
#if WASM_ENABLE_MULTI_MODULE != 0 || WASM_ENABLE_DYNAMIC_LINKING != 0
    printf("  --module-path=         Indicate a module search path. default is current\n"
           "                         directory('./')\n");
#endif
#if WASM_ENABLE_LIB_PTHREAD != 0
    printf("  --max-threads=n        Set maximum thread number per cluster, default is 4\n");
#endif
#if WASM_ENABLE_DEBUG_INTERP != 0
    printf("  -g=ip:port             Set the debug sever address, default is debug disabled\n");
    printf("                           if port is 0, then a random port will be used\n");
#endif
    return 1;
}
#if WASM_ENABLE_DYNAMIC_LINKING != 0
static void *
app_instance_program_main(wasm_program_t program, wasm_module_inst_t module_inst)
{
    const char *exception;
    if (program) {
        wasm_application_execute_program_main(program, app_argc, app_argv);
        if ((exception = wasm_runtime_get_program_exception(program)))
            printf("%s\n", exception);
    } else
    {
        wasm_application_execute_main(module_inst, app_argc, app_argv);
        if ((exception = wasm_runtime_get_exception(module_inst)))
            printf("%s\n", exception);
    }
    return NULL;
}

static void *
app_instance_program_func(wasm_program_t program, wasm_module_inst_t module_inst, const char *func_name)
{
    if (program)
        wasm_application_execute_program_func(program, func_name, app_argc - 1,
                                  app_argv + 1);
    else
        wasm_application_execute_func(module_inst, func_name, app_argc - 1,
                                  app_argv + 1);
    return NULL;
}
#endif
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
        res = (char **)realloc(res, sizeof(char *) * (uint32)(idx + 1));
        if (res == NULL) {
            return NULL;
        }
        res[idx++] = p;
    } while (p);

    /**
     * since the function name,
     * res[0] might be contains a '\' to indicate a space
     * func\name -> func name
     */
    p = strchr(res[0], '\\');
    while (p) {
        *p = ' ';
        p = strchr(p, '\\');
    }

    if (count) {
        *count = idx - 1;
    }
    return res;
}
#if WASM_ENABLE_DYNAMIC_LINKING != 0
static void *
app_instance_program_repl(wasm_program_t program, wasm_module_inst_t module_inst)
{
    char *cmd = NULL;
    size_t len = 0;
    ssize_t n;
    while ((printf("webassembly> "), n = getline(&cmd, &len, stdin)) != -1) {
        bh_assert(n > 0);
        if (cmd[n - 1] == '\n') {
            if (n == 1)
                continue;
            else
                cmd[n - 1] = '\0';
        }
        if (!strcmp(cmd, "__exit__")) {
            printf("exit repl mode\n");
            break;
        }
        app_argv = split_string(cmd, &app_argc);
        if (app_argv == NULL) {
            LOG_ERROR("Wasm prepare param failed: split string failed.\n");
            break;
        }
        if (app_argc != 0) {
            if (program)
                wasm_application_execute_program_func(program, app_argv[0],
                                          app_argc - 1, app_argv + 1);
            else
                wasm_application_execute_func(module_inst, app_argv[0],
                                          app_argc - 1, app_argv + 1);
        }
        free(app_argv);
    }
    free(cmd);
    return NULL;
}
#endif

static void *
app_instance_repl(wasm_module_inst_t module_inst)
{
    char *cmd = NULL;
    size_t len = 0;
    ssize_t n;

    while ((printf("webassembly> "), n = getline(&cmd, &len, stdin)) != -1) {
        bh_assert(n > 0);
        if (cmd[n - 1] == '\n') {
            if (n == 1)
                continue;
            else
                cmd[n - 1] = '\0';
        }
        if (!strcmp(cmd, "__exit__")) {
            printf("exit repl mode\n");
            break;
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

#if WASM_ENABLE_GLOBAL_HEAP_POOL != 0
#ifdef __NuttX__
static char global_heap_buf[WASM_GLOBAL_HEAP_SIZE * BH_KB] = { 0 };
#else
static char global_heap_buf[10 * 1024 * 1024] = { 0 };
#endif
#endif

#if WASM_ENABLE_MULTI_MODULE != 0 || WASM_ENABLE_DYNAMIC_LINKING != 0
static char *
handle_module_path(const char *module_path)
{
    /* next character after = */
    return (strchr(module_path, '=')) + 1;
}

static char * module_search_paths = NULL;

static bool
module_reader_callback(const char *module_name, uint8 **p_buffer,
                       uint32 *p_size)
{
#if WASM_ENABLE_DYNAMIC_LINKING != 0
    const char * format = "/%s";
    const char * format_with_path = "%s/%s";
#else
    const char * format = "/%s.wasm";
    const char * format_with_path = "%s/%s.wasm";
#endif
    const char * search_path = module_search_paths;
    char * file_full_path = NULL;
    int path_len = 0, buf_size = 0;
    *p_buffer = NULL;
    *p_size = 0;
    while (search_path) {
        char * end = strchr(search_path, ':');
        uint32 len = 0;
        if (end)
            len = end - search_path;
        else
            len = strlen(search_path);
        path_len = len + strlen("/") + strlen(module_name) +
             strlen(".wasm") + 1;
        if (path_len > buf_size) {
            if (file_full_path)
                wasm_runtime_free(file_full_path);
            buf_size = path_len;
            file_full_path = BH_MALLOC(buf_size);
            if (!file_full_path) {
        return false;
    }
        }

        memset(file_full_path, 0, buf_size);
        bh_memcpy_s(file_full_path, buf_size, search_path, len);
        snprintf(file_full_path + len, buf_size - len, format, module_name);

        *p_buffer = (uint8_t *)bh_read_file_to_buffer(file_full_path, p_size);

        if (*p_buffer)
            break;
        if (end) {
            search_path = end + 1;
            continue;
        }
        search_path = NULL;
    }
    if (!search_path) {
        bh_assert(!*p_buffer);
        if (!file_full_path) {
            buf_size = strlen("./") + strlen(module_name) +
                strlen(".wasm") + 1;
            file_full_path = BH_MALLOC(buf_size);
            if (!file_full_path) {
                return false;
            }
        }
        snprintf(file_full_path, buf_size, format_with_path, ".", module_name);
        *p_buffer = (uint8_t *)bh_read_file_to_buffer(file_full_path, p_size);
    }
    if (file_full_path)
        wasm_runtime_free(file_full_path);
    return (*p_buffer != NULL);
}

static void
module_destroyer_impl(uint8 *buffer, uint32 size)
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
    const char * file_name = NULL;
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
    bool is_repl_mode = false;
	bool is_xip_mode = false;
    bool is_standalone_mode = true;
    bool auto_ext_name = true;
#if WASM_ENABLE_DYNAMIC_LINKING != 0
    uint32 dlopen_mode = 0; // lazy binding | from builtin libc
#endif
#if WASM_ENABLE_LIBC_WASI != 0
    const char *dir_list[8] = { NULL };
    uint32 dir_list_size = 0;
    const char *env_list[8] = { NULL };
    uint32 env_list_size = 0;
#endif
#if WASM_ENABLE_DEBUG_INTERP != 0
    char *ip_addr = NULL;
    /* int platform_port = 0; */
    int instance_port = 0;
#endif

    /* Process options. */
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
        else if (!strcmp(argv[0], "--repl")) {
            is_repl_mode = true;
        }
		else if (!strcmp(argv[0], "--xip")) {
            is_xip_mode = true;
        }
#if WASM_ENABLE_DYNAMIC_LINKING != 0
        else if (!strncmp(argv[0], "--enable-dlopen=", 16)) {
            if (argv[0][16] == '\0')
                return print_help();
            is_standalone_mode = false;
            dlopen_mode = atoi(argv[0] + 16);
        }
        else if (!strcmp(argv[0], "--disable-auto-ext")) {
            auto_ext_name = false;
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
#if WASM_ENABLE_MULTI_MODULE != 0 || WASM_ENABLE_DYNAMIC_LINKING != 0
        else if (!strncmp(argv[0], MODULE_PATH, strlen(MODULE_PATH))) {
            module_search_paths = handle_module_path(argv[0]);
            if (!strlen(module_search_paths)) {
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
#if WASM_ENABLE_DEBUG_INTERP != 0
        else if (!strncmp(argv[0], "-g=", 3)) {
            char *port_str = strchr(argv[0] + 3, ':');
            char *port_end;
            if (port_str == NULL)
                return print_help();
            *port_str = '\0';
            instance_port = strtoul(port_str + 1, &port_end, 10);
            if (port_str[1] == '\0' || *port_end != '\0')
                return print_help();
            ip_addr = argv[0] + 3;
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
    init_args.standalone = is_standalone_mode;
    init_args.auto_ext_name = auto_ext_name;

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

#if WASM_ENABLE_DEBUG_INTERP != 0
    init_args.platform_port = 0;
    init_args.instance_port = instance_port;
    if (ip_addr)
        strcpy(init_args.ip_addr, ip_addr);
#endif

#if 0
#if AS_HARDCODE_ABORT != 0
    static NativeSymbol native_symbols[] =
    {
        {
            {"abort"}, 		// the name of WASM function name
            as_abort, 			// the native function pointer
            "(iiii)",			// the function prototype signature, avoid to use i32
            NULL                // attachment is NULL
        }
    };
    init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
    init_args.native_module_name = "env";
    init_args.native_symbols = native_symbols;
#endif
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

    if (is_xip_mode) {
        uint8 *wasm_file_mapped;
        int map_prot = MMAP_PROT_READ | MMAP_PROT_WRITE | MMAP_PROT_EXEC;
        int map_flags = MMAP_MAP_NONE;

        if (!(wasm_file_mapped =
                  os_mmap(NULL, (uint32)wasm_file_size, map_prot, map_flags))) {
            printf("mmap memory failed\n");
            wasm_runtime_free(wasm_file_buf);
            goto fail1;
        }

        bh_memcpy_s(wasm_file_mapped, wasm_file_size, wasm_file_buf,
                    wasm_file_size);
        wasm_runtime_free(wasm_file_buf);
        wasm_file_buf = wasm_file_mapped;
    }

#if WASM_ENABLE_MULTI_MODULE != 0
    wasm_runtime_set_module_reader(module_reader_callback, module_destroyer_impl);
#endif

    file_name = strrchr(wasm_file, '/');
    if (file_name)
        file_name ++;
    else
        file_name = wasm_file;
    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load2(file_name, wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail2;
    }
#if WASM_ENABLE_DYNAMIC_LINKING != 0
    if (!is_standalone_mode) {
        wasm_runtime_set_module_reader(module_reader_callback, module_destroyer_impl);
        wasm_program_t program = wasm_runtime_create_program(wasm_module,
                            stack_size, heap_size, dlopen_mode, error_buf, sizeof(error_buf));
        if (!program) {
            printf("%s\n", error_buf);
            return 0;
        }
        if (is_repl_mode)
            app_instance_program_repl(program, NULL);
        else if (func_name)
            app_instance_program_func(program, NULL, func_name);
        else
            app_instance_program_main(program, NULL);

        printf("%s\n", error_buf);

        wasm_runtime_destroy_program(program);

        /* free the file buffer */
        if (!is_xip_mode)
            wasm_runtime_free(wasm_file_buf);
        else
            os_munmap(wasm_file_buf, wasm_file_size);

        wasm_runtime_destroy();
        return 0;
    }
#endif

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
     wasm_runtime_unload2(wasm_module);

fail2:
    /* free the file buffer */
    if (!is_xip_mode)
        wasm_runtime_free(wasm_file_buf);
    else
        os_munmap(wasm_file_buf, wasm_file_size);

fail1:
    /* destroy runtime environment */
    wasm_runtime_destroy();
    return 0;
}
