/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#include "Enclave_t.h"
#include "wasm_export.h"
#include "bh_platform.h"

extern "C" {
    typedef void (*os_print_function_t)(const char* message);
    extern void os_set_print_function(os_print_function_t pf);

    void
    enclave_print(const char *message)
    {
        ocall_print(message);
    }
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

typedef struct EnclaveModule {
    wasm_module_t module;
    uint8 *wasm_file;
    uint32 wasm_file_size;
    char *wasi_arg_buf;
    char **wasi_dir_list;
    uint32 wasi_dir_list_size;
    char **wasi_env_list;
    uint32 wasi_env_list_size;
    char **wasi_argv;
    uint32 wasi_argc;
} EnclaveModule;

#if WASM_ENABLE_SPEC_TEST == 0
static char global_heap_buf[10 * 1024 * 1024] = { 0 };
#else
static char global_heap_buf[100 * 1024 * 1024] = { 0 };
#endif

static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL)
        snprintf(error_buf, error_buf_size, "%s", string);
}

static void
handle_cmd_init_runtime(uint64 *args, uint32 argc)
{
    bool alloc_with_pool;
    uint32 max_thread_num;
    RuntimeInitArgs init_args;

    bh_assert(argc == 2);

    os_set_print_function(enclave_print);

#if WASM_ENABLE_SPEC_TEST == 0
    alloc_with_pool = (bool)args[0];
#else
    alloc_with_pool = true;
#endif
    max_thread_num = (uint32)args[1];

    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.max_thread_num = max_thread_num;

    if (alloc_with_pool) {
        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
    }
    else {
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
    }

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        LOG_ERROR("Init runtime environment failed.\n");
        args[0] = false;
        return;
    }

    args[0] = true;

    LOG_VERBOSE("Init runtime environment success.\n");
}

static void
handle_cmd_destroy_runtime()
{
    wasm_runtime_destroy();

    LOG_VERBOSE("Destroy runtime success.\n");
}

static void
handle_cmd_load_module(uint64 *args, uint32 argc)
{
    uint64 *args_org = args;
    char *wasm_file = *(char **)args++;
    uint32 wasm_file_size = *(uint32 *)args++;
    char *error_buf = *(char **)args++;
    uint32 error_buf_size = *(uint32 *)args++;
    uint64 total_size = sizeof(EnclaveModule) + (uint64)wasm_file_size;
    EnclaveModule *enclave_module;

    bh_assert(argc == 4);

    if (total_size >= UINT32_MAX
        || !(enclave_module = (EnclaveModule *)
                    wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: "
                      "allocate memory failed.");
        *(void **)args_org = NULL;
        return;
    }

    memset(enclave_module, 0, (uint32)total_size);
    enclave_module->wasm_file = (uint8 *)enclave_module
                                + sizeof(EnclaveModule);
    bh_memcpy_s(enclave_module->wasm_file, wasm_file_size,
                wasm_file, wasm_file_size);

    if (!(enclave_module->module =
                wasm_runtime_load(enclave_module->wasm_file, wasm_file_size,
                                  error_buf, error_buf_size))) {
        wasm_runtime_free(enclave_module);
        *(void **)args_org = NULL;
        return;
    }

    *(EnclaveModule **)args_org = enclave_module;

    LOG_VERBOSE("Load module success.\n");
}

static void
handle_cmd_unload_module(uint64 *args, uint32 argc)
{
    EnclaveModule *enclave_module = *(EnclaveModule **)args++;
    uint32 i;

    bh_assert(argc == 1);

    if (enclave_module->wasi_arg_buf)
        wasm_runtime_free(enclave_module->wasi_arg_buf);

    wasm_runtime_unload(enclave_module->module);
    wasm_runtime_free(enclave_module);

    LOG_VERBOSE("Unload module success.\n");
}

static void
handle_cmd_instantiate_module(uint64 *args, uint32 argc)
{
    uint64 *args_org = args;
    EnclaveModule *enclave_module = *(EnclaveModule **)args++;
    uint32 stack_size = *(uint32 *)args++;
    uint32 heap_size = *(uint32 *)args++;
    char *error_buf = *(char **)args++;
    uint32 error_buf_size = *(uint32 *)args++;
    wasm_module_inst_t module_inst;

    bh_assert(argc == 5);

    if (!(module_inst =
                wasm_runtime_instantiate(enclave_module->module,
                                         stack_size, heap_size,
                                         error_buf, error_buf_size))) {
        *(void **)args_org = NULL;
        return;
    }

    *(wasm_module_inst_t *)args_org = module_inst;

    LOG_VERBOSE("Instantiate module success.\n");
}

static void
handle_cmd_deinstantiate_module(uint64 *args, uint32 argc)
{
    wasm_module_inst_t module_inst = *(wasm_module_inst_t *)args++;

    bh_assert(argc == 1);

    wasm_runtime_deinstantiate(module_inst);

    LOG_VERBOSE("Deinstantiate module success.\n");
}

static void
handle_cmd_get_exception(uint64 *args, uint32 argc)
{
    uint64 *args_org = args;
    wasm_module_inst_t module_inst = *(wasm_module_inst_t *)args++;
    char *exception = *(char **)args++;
    uint32 exception_size = *(uint32 *)args++;
    const char *exception1;

    bh_assert(argc == 3);

    if ((exception1 = wasm_runtime_get_exception(module_inst))) {
        snprintf(exception, exception_size,
                 "%s", exception1);
        args_org[0] = true;
    }
    else {
        args_org[0] = false;
    }
}

static void
handle_cmd_exec_app_main(uint64 *args, int32 argc)
{
    wasm_module_inst_t module_inst = *(wasm_module_inst_t *)args++;
    uint32 app_argc = *(uint32 *)args++;
    char **app_argv = NULL;
    uint64 total_size;
    int32 i;

    bh_assert(argc >= 3);
    bh_assert(app_argc >= 1);

    total_size = sizeof(char *) * (app_argc > 2 ? (uint64)app_argc : 2);

    if (total_size >= UINT32_MAX
        || !(app_argv = (char **)wasm_runtime_malloc(total_size))) {
        wasm_runtime_set_exception(module_inst, "allocate memory failed.");
        return;
    }

    for (i = 0; i < app_argc; i++) {
        app_argv[i] = (char *)(uintptr_t)args[i];
    }

    wasm_application_execute_main(module_inst, app_argc - 1, app_argv + 1);

    wasm_runtime_free(app_argv);
}

static void
handle_cmd_exec_app_func(uint64 *args, int32 argc)
{
    wasm_module_inst_t module_inst = *(wasm_module_inst_t *)args++;
    char *func_name = *(char **)args++;
    uint32 app_argc = *(uint32 *)args++;
    char **app_argv = NULL;
    uint64 total_size;
    int32 i, func_name_len = strlen(func_name);

    bh_assert(argc == app_argc + 3);

    total_size = sizeof(char *) * (app_argc > 2 ? (uint64)app_argc : 2);

    if (total_size >= UINT32_MAX
        || !(app_argv = (char **)wasm_runtime_malloc(total_size))) {
        wasm_runtime_set_exception(module_inst, "allocate memory failed.");
        return;
    }

    for (i = 0; i < app_argc; i++) {
        app_argv[i] = (char *)(uintptr_t)args[i];
    }

    wasm_application_execute_func(module_inst, func_name, app_argc, app_argv);

    wasm_runtime_free(app_argv);
}

static void
handle_cmd_set_log_level(uint64 *args, uint32 argc)
{
#if WASM_ENABLE_LOG != 0
    LOG_VERBOSE("Set log verbose level to %d.\n", (int)args[0]);
    bh_log_set_verbose_level((int)args[0]);
#endif
}

#ifndef SGX_DISABLE_WASI
static void
handle_cmd_set_wasi_args(uint64 *args, int32 argc)
{
    uint64 *args_org = args;
    EnclaveModule *enclave_module = *(EnclaveModule **)args++;
    char **dir_list = *(char ***)args++;
    uint32 dir_list_size = *(uint32 *)args++;
    char **env_list = *(char ***)args++;
    uint32 env_list_size = *(uint32 *)args++;
    char **wasi_argv = *(char ***)args++;
    char *p, *p1;
    uint32 wasi_argc = *(uint32 *)args++;
    uint64 total_size = 0;
    int32 i, str_len;

    bh_assert(argc == 7);

    total_size += sizeof(char *) * (uint64)dir_list_size
                  + sizeof(char *) * (uint64)env_list_size
                  + sizeof(char *) * (uint64)wasi_argc;

    for (i = 0; i < dir_list_size; i++) {
        total_size += strlen(dir_list[i]) + 1;
    }

    for (i = 0; i < env_list_size; i++) {
        total_size += strlen(env_list[i]) + 1;
    }

    for (i = 0; i < wasi_argc; i++) {
        total_size += strlen(wasi_argv[i]) + 1;
    }

    if (total_size >= UINT32_MAX
        || !(enclave_module->wasi_arg_buf = p = (char *)
                    wasm_runtime_malloc((uint32)total_size))) {
        *args_org = false;
        return;
    }

    p1 = p + sizeof(char *) * dir_list_size
           + sizeof(char *) * env_list_size
           + sizeof(char *) * wasi_argc;

    if (dir_list_size > 0) {
        enclave_module->wasi_dir_list = (char **)p;
        enclave_module->wasi_dir_list_size = dir_list_size;
        for (i = 0; i < dir_list_size; i++) {
            enclave_module->wasi_dir_list[i] = p1;
            str_len = strlen(dir_list[i]);
            bh_memcpy_s(p1, str_len + 1, dir_list[i], str_len + 1);
            p1 += str_len + 1;
        }
        p += sizeof(char *) * dir_list_size;
    }

    if (env_list_size > 0) {
        enclave_module->wasi_env_list = (char **)p;
        enclave_module->wasi_env_list_size = env_list_size;
        for (i = 0; i < env_list_size; i++) {
            enclave_module->wasi_env_list[i] = p1;
            str_len = strlen(env_list[i]);
            bh_memcpy_s(p1, str_len + 1, env_list[i], str_len + 1);
            p1 += str_len + 1;
        }
        p += sizeof(char *) * env_list_size;
    }

    if (wasi_argc > 0) {
        enclave_module->wasi_argv = (char **)p;
        enclave_module->wasi_argc = wasi_argc;
        for (i = 0; i < wasi_argc; i++) {
            enclave_module->wasi_argv[i] = p1;
            str_len = strlen(wasi_argv[i]);
            bh_memcpy_s(p1, str_len + 1, wasi_argv[i], str_len + 1);
            p1 += str_len + 1;
        }
        p += sizeof(char *) * wasi_argc;
    }

    wasm_runtime_set_wasi_args(enclave_module->module,
                               (const char **)enclave_module->wasi_dir_list,
                               dir_list_size,
                               NULL, 0,
                               (const char **)enclave_module->wasi_env_list,
                               env_list_size,
                               enclave_module->wasi_argv,
                               enclave_module->wasi_argc);

    *args_org = true;
}
#else
static void
handle_cmd_set_wasi_args(uint64 *args, int32 argc)
{
    *args = true;
}
#endif /* end of SGX_DISABLE_WASI */

void
ecall_handle_command(unsigned cmd,
                     unsigned char *cmd_buf,
                     unsigned cmd_buf_size)
{
    uint64 *args = (uint64 *)cmd_buf;
    uint32 argc = cmd_buf_size / sizeof(uint64);

    switch (cmd) {
        case CMD_INIT_RUNTIME:
            handle_cmd_init_runtime(args, argc);
            break;
        case CMD_LOAD_MODULE:
            handle_cmd_load_module(args, argc);
            break;
        case CMD_SET_WASI_ARGS:
            handle_cmd_set_wasi_args(args, argc);
            break;
        case CMD_INSTANTIATE_MODULE:
            handle_cmd_instantiate_module(args, argc);
            break;
        case CMD_LOOKUP_FUNCTION:
            break;
        case CMD_CREATE_EXEC_ENV:
            break;
        case CMD_CALL_WASM:
            break;
        case CMD_EXEC_APP_FUNC:
            handle_cmd_exec_app_func(args, argc);
            break;
        case CMD_EXEC_APP_MAIN:
            handle_cmd_exec_app_main(args, argc);
            break;
        case CMD_GET_EXCEPTION:
            handle_cmd_get_exception(args, argc);
            break;
        case CMD_DEINSTANTIATE_MODULE:
            handle_cmd_deinstantiate_module(args, argc);
            break;
        case CMD_UNLOAD_MODULE:
            handle_cmd_unload_module(args, argc);
            break;
        case CMD_DESTROY_RUNTIME:
            handle_cmd_destroy_runtime();
            break;
        case CMD_SET_LOG_LEVEL:
            handle_cmd_set_log_level(args, argc);
            break;
        default:
            LOG_ERROR("Unknown command %d\n", cmd);
            break;
    }
}

void
ecall_iwasm_main(uint8_t *wasm_file_buf, uint32_t wasm_file_size)
{
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    RuntimeInitArgs init_args;
    char error_buf[128];
    const char *exception;

    os_set_print_function(enclave_print);

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        ocall_print("Init runtime environment failed.");
        ocall_print("\n");
        return;
    }

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        ocall_print(error_buf);
        ocall_print("\n");
        goto fail1;
    }

    /* instantiate the module */
    if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module,
                                                      16 * 1024,
                                                      16 * 1024,
                                                      error_buf,
                                                      sizeof(error_buf)))) {
        ocall_print(error_buf);
        ocall_print("\n");
        goto fail2;
    }

    /* execute the main function of wasm app */
    wasm_application_execute_main(wasm_module_inst, 0, NULL);
    if ((exception = wasm_runtime_get_exception(wasm_module_inst))) {
        ocall_print(exception);
        ocall_print("\n");
    }

    /* destroy the module instance */
    wasm_runtime_deinstantiate(wasm_module_inst);

fail2:
    /* unload the module */
    wasm_runtime_unload(wasm_module);

fail1:
    /* destroy runtime environment */
    wasm_runtime_destroy();
}
