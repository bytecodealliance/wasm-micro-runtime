/*
 * Copyright (C) 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_log.h"
#include "wasmtime_ssp.h"
#include "thread_manager.h"

#if WASM_ENABLE_INTERP != 0
#include "wasm_runtime.h"
#endif

#if WASM_ENABLE_AOT != 0
#include "aot_runtime.h"
#endif

static const char *THREAD_START_FUNCTION = "wasi_thread_start";

static korp_mutex thread_id_lock;

typedef struct {
    /* app's entry function */
    wasm_function_inst_t start_func;
    /* arg of the app's entry function */
    uint32 arg;
    /* thread id passed to the app */
    int32 thread_id;
} ThreadStartArg;

static int32
allocate_thread_id()
{
    static int32 thread_id = 0;

    int32 id;

    os_mutex_lock(&thread_id_lock);
    id = thread_id++;
    os_mutex_unlock(&thread_id_lock);
    return id;
}

static void *
thread_start(void *arg)
{
    wasm_exec_env_t exec_env = (wasm_exec_env_t)arg;
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    ThreadStartArg *thread_arg = exec_env->thread_arg;
    uint32 argv[2];

    wasm_exec_env_set_thread_info(exec_env);
    argv[0] = thread_arg->thread_id;
    argv[1] = thread_arg->arg;

    if (!wasm_runtime_call_wasm(exec_env, thread_arg->start_func, 2, argv)) {
        if (wasm_runtime_get_exception(module_inst))
            wasm_cluster_spread_exception(exec_env);
    }

    /* routine exit, destroy instance */
    wasm_runtime_deinstantiate_internal(module_inst, true);

    wasm_runtime_free(thread_arg);
    exec_env->thread_arg = NULL;

    return NULL;
}

static int32
thread_spawn_wrapper(wasm_exec_env_t exec_env, uint32 start_arg)
{
    wasm_module_t module = wasm_exec_env_get_module(exec_env);
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    wasm_module_inst_t new_module_inst = NULL;
    ThreadStartArg *thread_start_arg = NULL;
    wasm_function_inst_t start_func;
    int32 thread_id;
    uint32 stack_size = 8192;
    int32 ret = -1;
#if WASM_ENABLE_LIBC_WASI != 0
    WASIContext *wasi_ctx;
#endif

    bh_assert(module);
    bh_assert(module_inst);

    stack_size = ((WASMModuleInstance *)module_inst)->default_wasm_stack_size;

    if (!(new_module_inst = wasm_runtime_instantiate_internal(
              module, true, stack_size, 0, NULL, 0)))
        return -1;

    wasm_runtime_set_custom_data_internal(
        new_module_inst, wasm_runtime_get_custom_data(module_inst));

#if WASM_ENABLE_LIBC_WASI != 0
    wasi_ctx = wasm_runtime_get_wasi_ctx(module_inst);
    if (wasi_ctx)
        wasm_runtime_set_wasi_ctx(new_module_inst, wasi_ctx);
#endif

    start_func = wasm_runtime_lookup_function(new_module_inst,
                                              THREAD_START_FUNCTION, NULL);
    if (!start_func) {
        LOG_ERROR("Failed to find thread start function %s",
                  THREAD_START_FUNCTION);
        goto thread_spawn_fail;
    }

    if (!(thread_start_arg = wasm_runtime_malloc(sizeof(ThreadStartArg)))) {
        LOG_ERROR("Runtime args allocation failed");
        goto thread_spawn_fail;
    }

    thread_start_arg->thread_id = thread_id = allocate_thread_id();
    thread_start_arg->arg = start_arg;
    thread_start_arg->start_func = start_func;

    os_mutex_lock(&exec_env->wait_lock);
    ret = wasm_cluster_create_thread(exec_env, new_module_inst, thread_start,
                                     thread_start_arg);
    if (ret != 0) {
        os_mutex_unlock(&exec_env->wait_lock);
        LOG_ERROR("Failed to spawn a new thread");
        goto thread_spawn_fail;
    }
    os_mutex_unlock(&exec_env->wait_lock);

    return thread_id;

thread_spawn_fail:
    if (new_module_inst)
        wasm_runtime_deinstantiate_internal(new_module_inst, true);

    if (thread_start_arg)
        wasm_runtime_free(thread_start_arg);

    return -1;
}

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, func_name##_wrapper, signature, NULL }
/* clang-format on */

static NativeSymbol native_symbols_lib_wasi_threads[] = { REG_NATIVE_FUNC(
    thread_spawn, "(i)i") };

uint32
get_lib_wasi_threads_export_apis(NativeSymbol **p_lib_wasi_threads_apis)
{
    *p_lib_wasi_threads_apis = native_symbols_lib_wasi_threads;
    return sizeof(native_symbols_lib_wasi_threads) / sizeof(NativeSymbol);
}

bool
lib_wasi_threads_init(void)
{
    if (0 != os_mutex_init(&thread_id_lock))
        return false;

    return true;
}

void
lib_wasi_threads_destroy(void)
{
    os_mutex_destroy(&thread_id_lock);
}
