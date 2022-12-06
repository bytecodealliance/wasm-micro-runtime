/*
 * Copyright (C) 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasmtime_ssp.h"

#if WASM_ENABLE_INTERP != 0
#include "wasm_runtime.h"
#endif

#if WASM_ENABLE_AOT != 0
#include "aot_runtime.h"
#endif

static __wasi_errno_t
thread_spawn_wrapper(wasm_exec_env_t exec_env, void *start_arg)
{
    return __WASI_ENOSYS;
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
