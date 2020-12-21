/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <assert.h>
#include <iostream>
#include <cstdio>
#include <cstring>

#include "Enclave_u.h"
#include "sgx_urts.h"
#include "pal_api.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define TOKEN_FILENAME   "enclave.token"
#define ENCLAVE_FILENAME "enclave.signed.so"
#define MAX_PATH 1024

#define TEST_OCALL_API 0

static sgx_enclave_id_t g_eid = 0;

sgx_enclave_id_t
pal_get_enclave_id(void)
{
    return g_eid;
}

void
ocall_print(const char* str)
{
    printf("%s", str);
}

static char *
get_exe_path(char *path_buf, unsigned path_buf_size)
{
    ssize_t i;
    ssize_t size = readlink("/proc/self/exe",
                            path_buf, path_buf_size - 1);

    if (size < 0 || (size >= path_buf_size - 1)) {
        return NULL;
    }

    path_buf[size] = '\0';
    for (i = size - 1; i >= 0; i--) {
        if (path_buf[i] == '/') {
            path_buf[i + 1] = '\0';
            break;
        }
    }
    return path_buf;
}

/* Initialize the enclave:
 *   Step 1: try to retrieve the launch token saved by last transaction
 *   Step 2: call sgx_create_enclave to initialize an enclave instance
 *   Step 3: save the launch token if it is updated
 */
static int
enclave_init(sgx_enclave_id_t *p_eid)

{
    char token_path[MAX_PATH] = { '\0' };
    char enclave_path[MAX_PATH] = { '\0' };
    const char *home_dir;
    sgx_launch_token_t token = { 0 };
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int updated = 0;
    size_t write_num, enc_file_len;
    FILE *fp;

    enc_file_len = strlen(ENCLAVE_FILENAME);
    if (!get_exe_path(enclave_path, sizeof(enclave_path) - enc_file_len)) {
        printf("Failed to get exec path\n");
        return -1;
    }
    memcpy(enclave_path + strlen(enclave_path), ENCLAVE_FILENAME, enc_file_len);

    /* Step 1: try to retrieve the launch token saved by last transaction
     *         if there is no token, then create a new one.
     */
    /* try to get the token saved in $HOME */
    home_dir = getpwuid(getuid())->pw_dir;

    if (home_dir != NULL &&
        (strlen(home_dir) + strlen("/") + sizeof(TOKEN_FILENAME) + 1) <= MAX_PATH) {
        /* compose the token path */
        strncpy(token_path, home_dir, strlen(home_dir));
        strncat(token_path, "/", strlen("/"));
        strncat(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME) + 1);
    }
    else {
        /* if token path is too long or $HOME is NULL */
        strncpy(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
    }

    fp = fopen(token_path, "rb");
    if (fp == NULL && (fp = fopen(token_path, "wb")) == NULL) {
        printf("Warning: Failed to create/open the launch token file \"%s\".\n",
               token_path);
    }

    if (fp != NULL) {
        /* read the token from saved file */
        size_t read_num = fread(token, 1, sizeof(sgx_launch_token_t), fp);
        if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {
            /* if token is invalid, clear the buffer */
            memset(&token, 0x0, sizeof(sgx_launch_token_t));
            printf("Warning: Invalid launch token read from \"%s\".\n", token_path);
        }
    }

    /* Step 2: call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG,
                             &token, &updated, p_eid, NULL);
    if (ret != SGX_SUCCESS)
        /* Try to load enclave.sign.so from the path of exe file */
        ret = sgx_create_enclave(enclave_path, SGX_DEBUG_FLAG,
                                 &token, &updated, p_eid, NULL);
    if (ret != SGX_SUCCESS) {
        printf("Failed to create enclave from %s, error code: %d\n",
               ENCLAVE_FILENAME, ret);
        if (fp != NULL)
            fclose(fp);
        return -1;
    }

    /* Step 3: save the launch token if it is updated */
    if (updated == FALSE || fp == NULL) {
        /* if the token is not updated, or file handler is invalid, do not perform saving */
        if (fp != NULL) fclose(fp);
        return 0;
    }

    /* reopen the file with write capablity */
    fp = freopen(token_path, "wb", fp);
    if (fp == NULL)
        return 0;

    write_num = fwrite(token, 1, sizeof(sgx_launch_token_t), fp);
    if (write_num != sizeof(sgx_launch_token_t))
        printf("Warning: Failed to save launch token to \"%s\".\n", token_path);

    fclose(fp);
    return 0;
}

static unsigned char *
read_file_to_buffer(const char *filename, uint32_t *ret_size)
{
    unsigned char *buffer;
    FILE *file;
    int file_size, read_size;

    if (!filename || !ret_size) {
        printf("Read file to buffer failed: invalid filename or ret size.\n");
        return NULL;
    }

    if (!(file = fopen(filename, "r"))) {
        printf("Read file to buffer failed: open file %s failed.\n",
               filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (!(buffer = (unsigned char*)malloc(file_size))) {
        printf("Read file to buffer failed: alloc memory failed.\n");
        fclose(file);
        return NULL;
    }

    read_size = fread(buffer, 1, file_size, file);
    fclose(file);

    if (read_size < file_size) {
        printf("Read file to buffer failed: read file content failed.\n");
        free(buffer);
        return NULL;
    }

    *ret_size = file_size;

    return buffer;
}

static int
print_help()
{
    printf("Usage: iwasm [-options] wasm_file [args...]\n");
    printf("options:\n");
    printf("  -f|--function name     Specify a function name of the module to run rather\n"
           "                         than main\n");
    printf("  -v=n                   Set log verbose level (0 to 5, default is 2) larger\n"
           "                         level with more log\n");
    printf("  --stack-size=n         Set maximum stack size in bytes, default is 16 KB\n");
    printf("  --heap-size=n          Set maximum heap size in bytes, default is 16 KB\n");
    printf("  --repl                 Start a very simple REPL (read-eval-print-loop) mode\n"
           "                         that runs commands in the form of `FUNC ARG...`\n");
    printf("  --env=<env>            Pass wasi environment variables with \"key=value\"\n");
    printf("                         to the program, for example:\n");
    printf("                           --env=\"key1=value1\" --env=\"key2=value2\"\n");
    printf("  --dir=<dir>            Grant wasi access to the given host directories\n");
    printf("                         to the program, for example:\n");
    printf("                           --dir=<dir1> --dir=<dir2>\n");
    printf("  --max-threads=n        Set maximum thread number per cluster, default is 4\n");
    return 1;
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
        res = (char **)realloc(res, sizeof(char *) * (unsigned)(idx + 1));
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

typedef enum EcallCmd {
    CMD_INIT_RUNTIME = 0,     /* wasm_runtime_init/full_init() */
    CMD_LOAD_MODULE,          /* wasm_runtime_load() */
    CMD_INSTANTIATE_MODULE,   /* wasm_runtime_instantiate() */
    CMD_LOOKUP_FUNCTION,      /* wasm_runtime_lookup_function() */
    CMD_CREATE_EXEC_ENV,      /* wasm_runtime_create_exec_env() */
    CMD_CALL_WASM,            /* wasm_runtime_call_wasm */
    CMD_EXEC_APP_FUNC,        /* wasm_application_execute_func() */
    CMD_EXEC_APP_MAIN,        /* wasm_application_execute_main() */
    CMD_GET_EXCEPTION,        /* wasm_runtime_get_exception() */
    CMD_DEINSTANTIATE_MODULE, /* wasm_runtime_deinstantiate() */
    CMD_UNLOAD_MODULE,        /* wasm_runtime_unload() */
    CMD_DESTROY_RUNTIME,      /* wasm_runtime_destroy() */
    CMD_SET_WASI_ARGS,        /* wasm_runtime_set_wasi_args() */
    CMD_SET_LOG_LEVEL,        /* bh_log_set_verbose_level() */
} EcallCmd;

static void
app_instance_func(void *wasm_module_inst, const char *func_name,
                  int app_argc, char **app_argv);

static void *
app_instance_repl(void *module_inst, int app_argc, char **app_argv)
{
    char *cmd = NULL;
    size_t len = 0;
    ssize_t n;

    while ((printf("webassembly> "), n = getline(&cmd, &len, stdin)) != -1) {
        assert(n > 0);
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
            printf("Wasm prepare param failed: split string failed.\n");
            break;
        }
        if (app_argc != 0) {
            app_instance_func(module_inst, app_argv[0],
                              app_argc - 1, app_argv + 1);
        }
        free(app_argv);
    }
    free(cmd);
    return NULL;
}

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

static bool
set_log_verbose_level(int log_verbose_level)
{
    uint64_t ecall_args[1];

    /* Set log verbose level */
    if (log_verbose_level != 2) {
        ecall_args[0] = log_verbose_level;
        if (SGX_SUCCESS != ecall_handle_command(g_eid, CMD_SET_LOG_LEVEL,
                                                (uint8_t *)ecall_args,
                                                sizeof(uint64_t))) {
            printf("Call ecall_handle_command() failed.\n");
            return false;
        }
    }
    return true;
}

static bool
init_runtime(bool alloc_with_pool, uint32_t max_thread_num)
{
    uint64_t ecall_args[2];

    ecall_args[0] = alloc_with_pool;
    ecall_args[1] = max_thread_num;
    if (SGX_SUCCESS != ecall_handle_command(g_eid, CMD_INIT_RUNTIME,
                                            (uint8_t *)ecall_args,
                                            sizeof(uint64_t) * 2)) {
        printf("Call ecall_handle_command() failed.\n");
        return false;
    }
    if (!(bool)ecall_args[0]) {
        printf("Init runtime environment failed.\n");
        return false;
    }
    return true;
}

static void
destroy_runtime()
{
    if (SGX_SUCCESS != ecall_handle_command(g_eid, CMD_DESTROY_RUNTIME,
                                            NULL, 0)) {
        printf("Call ecall_handle_command() failed.\n");
    }
}

static void *
load_module(uint8_t *wasm_file_buf, uint32_t wasm_file_size,
            char *error_buf, uint32_t error_buf_size)
{
    uint64_t ecall_args[4];

    ecall_args[0] = (uint64_t)(uintptr_t)wasm_file_buf;
    ecall_args[1] = wasm_file_size;
    ecall_args[2] = (uint64_t)(uintptr_t)error_buf;
    ecall_args[3] = error_buf_size;
    if (SGX_SUCCESS != ecall_handle_command(g_eid, CMD_LOAD_MODULE,
                                            (uint8_t *)ecall_args,
                                            sizeof(uint64_t) * 4)) {
        printf("Call ecall_handle_command() failed.\n");
        return NULL;
    }

    return (void *)(uintptr_t)ecall_args[0];
}

static void
unload_module(void *wasm_module)
{
    uint64_t ecall_args[1];

    ecall_args[0] = (uint64_t)(uintptr_t)wasm_module;
    if (SGX_SUCCESS != ecall_handle_command(g_eid, CMD_UNLOAD_MODULE,
                                            (uint8_t *)ecall_args,
                                            sizeof(uint64_t))) {
        printf("Call ecall_handle_command() failed.\n");
    }
}

static void *
instantiate_module(void *wasm_module,
                   uint32_t stack_size, uint32_t heap_size,
                   char *error_buf, uint32_t error_buf_size)
{
    uint64_t ecall_args[5];

    ecall_args[0] = (uint64_t)(uintptr_t)wasm_module;
    ecall_args[1] = stack_size;
    ecall_args[2] = heap_size;
    ecall_args[3] = (uint64_t)(uintptr_t)error_buf;
    ecall_args[4] = error_buf_size;
    if (SGX_SUCCESS != ecall_handle_command(g_eid, CMD_INSTANTIATE_MODULE,
                                            (uint8_t *)ecall_args,
                                            sizeof(uint64_t) * 5)) {
        printf("Call ecall_handle_command() failed.\n");
        return NULL;
    }

    return (void *)(uintptr_t)ecall_args[0];
}

static void
deinstantiate_module(void *wasm_module_inst)
{
    uint64_t ecall_args[1];

    ecall_args[0] = (uint64_t)(uintptr_t)wasm_module_inst;
    if (SGX_SUCCESS != ecall_handle_command(g_eid, CMD_DEINSTANTIATE_MODULE,
                                            (uint8_t *)ecall_args,
                                            sizeof(uint64_t))) {
        printf("Call ecall_handle_command() failed.\n");
    }
}

static bool
get_exception(void *wasm_module_inst,
              char *exception, uint32_t exception_size)
{
    uint64_t ecall_args[3];

    ecall_args[0] = (uint64_t)(uintptr_t)wasm_module_inst;
    ecall_args[1] = (uint64_t)(uintptr_t)exception;
    ecall_args[2] = exception_size;
    if (SGX_SUCCESS != ecall_handle_command(g_eid, CMD_GET_EXCEPTION,
                                            (uint8_t *)ecall_args,
                                            sizeof(uint64_t) * 3)) {
        printf("Call ecall_handle_command() failed.\n");
    }

    return (bool)ecall_args[0];
}

static void
app_instance_main(void *wasm_module_inst,
                  int app_argc, char **app_argv)
{
    char exception[128];
    uint64_t ecall_args_buf[16], *ecall_args = ecall_args_buf;
    int i, size;

    if (app_argc + 2 > sizeof(ecall_args_buf) / sizeof(uint64_t)) {
        if (!(ecall_args = (uint64_t *)
                           malloc(sizeof(uint64_t) * (app_argc + 2)))) {
            printf("Allocate memory failed.\n");
            return;
        }
    }

    ecall_args[0] = (uintptr_t)wasm_module_inst;
    ecall_args[1] = app_argc;
    for (i = 0; i < app_argc; i++) {
        ecall_args[i + 2] = (uintptr_t)app_argv[i];
    }

    size = (uint32_t)sizeof(uint64_t) * (app_argc + 2);
    if (SGX_SUCCESS != ecall_handle_command(g_eid, CMD_EXEC_APP_MAIN,
                                            (uint8_t *)ecall_args,
                                            size)) {
        printf("Call ecall_handle_command() failed.\n");
    }

    if (get_exception(wasm_module_inst, exception, sizeof(exception))) {
        printf("%s\n", exception);
    }

    if (ecall_args != ecall_args_buf) {
        free(ecall_args);
    }
}

static void
app_instance_func(void *wasm_module_inst, const char *func_name,
                  int app_argc, char **app_argv)
{
    uint64_t ecall_args_buf[16], *ecall_args = ecall_args_buf;
    int i, size;

    if (app_argc + 3 > sizeof(ecall_args_buf) / sizeof(uint64_t)) {
        if (!(ecall_args = (uint64_t *)
                           malloc(sizeof(uint64_t) * (app_argc + 3)))) {
            printf("Allocate memory failed.\n");
            return;
        }
    }

    ecall_args[0] = (uintptr_t)wasm_module_inst;
    ecall_args[1] = (uintptr_t)func_name;
    ecall_args[2] = (uintptr_t)app_argc;
    for (i = 0; i < app_argc; i++) {
        ecall_args[i + 3] = (uintptr_t)app_argv[i];
    }

    size = (uint32_t)sizeof(uint64_t) * (app_argc + 3);
    if (SGX_SUCCESS != ecall_handle_command(g_eid, CMD_EXEC_APP_FUNC,
                                            (uint8_t *)ecall_args,
                                            size)) {
        printf("Call ecall_handle_command() failed.\n");
    }

    if (ecall_args != ecall_args_buf) {
        free(ecall_args);
    }
}

static bool
set_wasi_args(void *wasm_module,
              const char **dir_list, uint32_t dir_list_size,
              const char **env_list, uint32_t env_list_size,
              char **argv, uint32_t argc)
{
    uint64_t ecall_args[7];

    ecall_args[0] = (uint64_t)(uintptr_t)wasm_module;
    ecall_args[1] = (uint64_t)(uintptr_t)dir_list;
    ecall_args[2] = dir_list_size;
    ecall_args[3] = (uint64_t)(uintptr_t)env_list;
    ecall_args[4] = env_list_size;
    ecall_args[5] = (uint64_t)(uintptr_t)argv;
    ecall_args[6] = argc;
    if (SGX_SUCCESS != ecall_handle_command(g_eid, CMD_SET_WASI_ARGS,
                                            (uint8_t *)ecall_args,
                                            sizeof(uint64_t) * 7)) {
        printf("Call ecall_handle_command() failed.\n");
    }

    return (bool)ecall_args[0];
}

int
main(int argc, char *argv[])
{
    char *wasm_file = NULL;
    const char *func_name = NULL;
    uint8_t *wasm_file_buf = NULL;
    uint32_t wasm_file_size;
    uint32_t stack_size = 16 * 1024, heap_size = 16 * 1024;
    void *wasm_module = NULL;
    void *wasm_module_inst = NULL;
    char error_buf[128] = { 0 };
    int log_verbose_level = 2;
    bool is_repl_mode = false, alloc_with_pool = false;
    const char *dir_list[8] = { NULL };
    uint32_t dir_list_size = 0;
    const char *env_list[8] = { NULL };
    uint32_t env_list_size = 0;
    uint32_t max_thread_num = 4;

    if (enclave_init(&g_eid) < 0) {
        std::cout << "Fail to initialize enclave." << std::endl;
        return 1;
    }

#if TEST_OCALL_API != 0
    {
        if (!init_runtime(alloc_with_pool, max_thread_num)) {
            return -1;
        }
        ecall_iwasm_test(g_eid);
        destroy_runtime();
        return 0;
    }
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
        else if (!strncmp(argv[0], "-v=", 3)) {
            log_verbose_level = atoi(argv[0] + 3);
            if (log_verbose_level < 0 || log_verbose_level > 5)
                return print_help();
        }
        else if (!strcmp(argv[0], "--repl")) {
            is_repl_mode = true;
        }
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
        else if (!strncmp(argv[0], "--max-threads=", 14)) {
            if (argv[0][14] == '\0')
                return print_help();
            max_thread_num = atoi(argv[0] + 14);
        }
        else
            return print_help();
    }

    if (argc == 0)
        return print_help();

    wasm_file = argv[0];

    /* Init runtime */
    if (!init_runtime(alloc_with_pool, max_thread_num)) {
        return -1;
    }

    /* Set log verbose level */
    if (!set_log_verbose_level(log_verbose_level)) {
        goto fail1;
    }

    /* Load WASM byte buffer from WASM bin file */
    if (!(wasm_file_buf =
            (uint8_t *)read_file_to_buffer(wasm_file, &wasm_file_size))) {
        goto fail1;
    }

    /* Load module */
    if (!(wasm_module = load_module(wasm_file_buf, wasm_file_size,
                                    error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail2;
    }

    /* Set wasi arguments */
    if (!set_wasi_args(wasm_module, dir_list, dir_list_size,
                       env_list, env_list_size, argv, argc)) {
        printf("%s\n", "set wasi arguments failed.\n");
        goto fail3;
    }

    /* Instantiate module */
    if (!(wasm_module_inst = instantiate_module(wasm_module,
                                                stack_size, heap_size,
                                                error_buf,
                                                sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail3;
    }

    if (is_repl_mode)
        app_instance_repl(wasm_module_inst, argc, argv);
    else if (func_name)
        app_instance_func(wasm_module_inst, func_name,
                          argc - 1, argv + 1);
    else
        app_instance_main(wasm_module_inst, argc, argv);

    /* Deinstantiate module */
    deinstantiate_module(wasm_module_inst);

fail3:
    /* Unload module */
    unload_module(wasm_module);

fail2:
    /* Free the file buffer */
    free(wasm_file_buf);

fail1:
    /* Destroy runtime environment */
    destroy_runtime();

    return 0;
}

int
wamr_pal_get_version(void)
{
    return WAMR_PAL_VERSION;
}

int
wamr_pal_init(const struct wamr_pal_attr *args)
{
    sgx_enclave_id_t *p_eid = &g_eid;

    if (enclave_init(&g_eid) < 0) {
        std::cout << "Fail to initialize enclave." << std::endl;
        return 1;
    }
}

int
wamr_pal_create_process(struct wamr_pal_create_process_args *args)
{
    uint32_t stack_size = 16 * 1024, heap_size = 16 * 1024;
    int log_verbose_level = 2;
    bool is_repl_mode = false, alloc_with_pool = false;
    const char *dir_list[8] = { NULL };
    uint32_t dir_list_size = 0;
    const char *env_list[8] = { NULL };
    uint32_t env_list_size = 0;
    uint32_t max_thread_num = 4;
    char *wasm_files[16];
    void *wasm_module_inst[16];

    int argc = 2;
    char *argv[argc] = { (char*)"./iwasm", (char *)args->argv[0] };

    uint8_t *wasm_files_buf = NULL;
    void *wasm_modules = NULL;
    int len = 0, i;

    char *temp = (char *)args->argv[0];
    while(temp) {
        len++;
        temp=(char *)args->argv[len];
    }

    if (len > sizeof(wasm_files)/sizeof(char *)) {
        printf("Number of input files is out of range\n");
        return -1;
    }

    for (i = 0; i < len; ++i) {
        wasm_files[i] = (char *)args->argv[i];
    }

    /* Init runtime */
    if (!init_runtime(alloc_with_pool, max_thread_num)) {
        printf("Failed to init runtime\n");
        return -1;
    }

    /* Set log verbose level */
    if (!set_log_verbose_level(log_verbose_level)) {
        printf("Failed to set log level\n");
        destroy_runtime();
        return -1;
    }

    for (i = 0; i < len; ++i) {
        uint8_t *wasm_file_buf = NULL;
        uint32_t wasm_file_size;
        void *wasm_module = NULL;
        char error_buf[128] = { 0 };

        /* Load WASM byte buffer from WASM bin file */
        if (!(wasm_file_buf = (uint8_t *)read_file_to_buffer
                              (wasm_files[i], &wasm_file_size))) {
            printf("Failed to read file to buffer\n");
            destroy_runtime();
            return -1;
        }

        /* Load module */
        if (!(wasm_module = load_module(wasm_file_buf, wasm_file_size,
                                        error_buf, sizeof(error_buf)))) {
            printf("%s\n", error_buf);
            free(wasm_file_buf);
            destroy_runtime();
            return -1;
        }

        /* Set wasi arguments */
        if (!set_wasi_args(wasm_module, dir_list, dir_list_size,
                           env_list, env_list_size, argv, argc)) {
            printf("%s\n", "set wasi arguments failed.\n");
            unload_module(wasm_module);
            free(wasm_file_buf);
            destroy_runtime();
            return -1;
        }

        /* Instantiate module */
        if (!(wasm_module_inst[i] = instantiate_module(wasm_module,
                                                stack_size, heap_size,
                                                error_buf,
                                                sizeof(error_buf)))) {
            printf("%s\n", error_buf);
            unload_module(wasm_module);
            free(wasm_file_buf);
            destroy_runtime();
            return -1;
        }

        app_instance_main(wasm_module_inst[i], argc, argv);

        /* Deinstantiate module */
        deinstantiate_module(wasm_module_inst[i]);
        unload_module(wasm_module);
        free(wasm_file_buf);
    }

    destroy_runtime();
    return 0;
}

int
wamr_pal_destroy(void)
{
    //sgx_destroy_enclave(g_eid);
    return 0;
}

int
wamr_pal_exec(struct wamr_pal_exec_args *args)
{
    //app_instance_main(wasm_module_inst[i], argc, argv);
    return 0;
}

int
wamr_pal_kill(int pid, int sig)
{
    //deinstantiate_module(wasm_module_inst[i]);
    //unload_module(wasm_module);
    //free(wasm_file_buf);
    return 0;
}

int pal_get_version(void) __attribute__((weak, alias ("wamr_pal_get_version")));

int pal_init(const struct wamr_pal_attr *attr)\
__attribute__ ((weak, alias ("wamr_pal_init")));

int pal_create_process(struct wamr_pal_create_process_args *args)\
__attribute__ ((weak, alias ("wamr_pal_create_process")));

int pal_exec(struct wamr_pal_exec_args *args)\
__attribute__ ((weak, alias ("wamr_pal_exec")));

int pal_kill(int pid, int sig) __attribute__ ((weak, alias ("wamr_pal_kill")));

int pal_destroy(void) __attribute__ ((weak, alias ("wamr_pal_destroy")));
