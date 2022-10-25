/*
 * Copyright (C) 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#if WASM_ENABLE_INTERP != 0
#include "wasm_runtime.h"
#endif

#if WASM_ENABLE_AOT != 0
#include "aot_runtime.h"
#endif

const char *THREAD_START_FUNCTION = "wasi_thread_start";

typedef struct {
    void *start_arg;
    wasm_exec_env_t exec_env;
    wasm_function_inst_t start_func;
} ThreadInfo;

static void *
thread_start(void *arg)
{
    ThreadInfo *info = arg;
    wasm_exec_env_t exec_env = info->exec_env;
    wasm_module_inst_t module_inst = get_module_inst(info->exec_env);
    uint32 argv[2];

    os_thread_signal_init(NULL);

    argv[0] = pthread_self();
    argv[1] = (uint32)(uintptr_t)info->start_arg;

    wasm_runtime_free(arg);
    if (!wasm_runtime_call_wasm(exec_env, info->start_func, 2, argv)) {
        // TODO action here
    }

    wasm_exec_env_destroy(exec_env);
    wasm_runtime_deinstantiate_internal(module_inst, true);

    return NULL;
}

static WASMExecEnv *
wasi_threads_create_exec_env(WASMExecEnv *exec_env)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    wasm_module_t module;
    wasm_module_inst_t new_module_inst;
#if WASM_ENABLE_LIBC_WASI != 0
    WASIContext *wasi_ctx;
#endif
    WASMExecEnv *new_exec_env;
    uint32 stack_size = 8192;

    if (!module_inst || !(module = wasm_exec_env_get_module(exec_env))) {
        return NULL;
    }

#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        stack_size =
            ((WASMModuleInstance *)module_inst)->default_wasm_stack_size;
    }
#endif

#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        stack_size =
            ((AOTModuleInstance *)module_inst)->default_wasm_stack_size;
    }
#endif

    if (!(new_module_inst = wasm_runtime_instantiate_internal(
              module, true, stack_size, 0, NULL, 0))) {
        return NULL;
    }

    /* Set custom_data to new module instance */
    wasm_runtime_set_custom_data_internal(
        new_module_inst, wasm_runtime_get_custom_data(module_inst));

#if WASM_ENABLE_LIBC_WASI != 0
    wasi_ctx = wasm_runtime_get_wasi_ctx(module_inst);
    wasm_runtime_set_wasi_ctx(new_module_inst, wasi_ctx);
#endif

    new_exec_env =
        wasm_exec_env_create(new_module_inst, exec_env->wasm_stack_size);
    if (!new_exec_env)
        goto fail1;

    return new_exec_env;

fail1:
    wasm_exec_env_destroy(new_exec_env);
    wasm_runtime_deinstantiate_internal(new_module_inst, true);

    return NULL;
}

static int
thread_spawn_wrapper(wasm_exec_env_t exec_env, uint32 *start_arg)
{
    wasm_module_inst_t new_module_inst;
    wasm_exec_env_t new_exec_env;
    wasm_function_inst_t start_func;
    ThreadInfo *info = NULL;
    pthread_t th;
    int ret = 0;

    new_exec_env = wasi_threads_create_exec_env(exec_env);
    if (!new_exec_env)
        return -1;

    new_module_inst = get_module_inst(new_exec_env);

    start_func = wasm_runtime_lookup_function(new_module_inst, THREAD_START_FUNCTION, NULL);
    if (!start_func) {
        ret = -1;
        goto fail;
    }

    info = wasm_runtime_malloc(sizeof(ThreadInfo));
    if (!info)
        goto fail;


    info->start_arg = start_arg;
    info->start_func = start_func;
    info->exec_env = new_exec_env;

    ret = pthread_create(&th, NULL, thread_start, info);
    if (ret != 0)
        goto fail;

    return ret;

fail:
    if (info)
        wasm_runtime_free(info);

    wasm_exec_env_destroy(new_exec_env);

    return ret;
}

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, func_name##_wrapper, signature, NULL }
/* clang-format on */

static NativeSymbol native_symbols_lib_wasi_threads[] = { REG_NATIVE_FUNC(
    thread_spawn, "(*)i") };

uint32
get_lib_wasi_threads_export_apis(NativeSymbol **p_lib_wasi_threads_apis)
{
    *p_lib_wasi_threads_apis = native_symbols_lib_wasi_threads;
    return sizeof(native_symbols_lib_wasi_threads) / sizeof(NativeSymbol);
}
