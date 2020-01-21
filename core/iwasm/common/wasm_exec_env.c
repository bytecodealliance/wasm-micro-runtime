/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_exec_env.h"
#include "bh_memory.h"
#include "wasm_runtime_common.h"

WASMExecEnv *
wasm_exec_env_create(struct WASMModuleInstanceCommon *module_inst,
                     uint32 stack_size)
{
    uint64 total_size = offsetof(WASMExecEnv, wasm_stack.s.bottom)
                        + (uint64)stack_size;
    WASMExecEnv *exec_env;

    if (total_size >= UINT32_MAX
        || !(exec_env = wasm_malloc((uint32)total_size)))
        return NULL;

    memset(exec_env, 0, (uint32)total_size);
    exec_env->module_inst = module_inst;
    exec_env->wasm_stack_size = stack_size;
    exec_env->wasm_stack.s.top_boundary =
        exec_env->wasm_stack.s.bottom + stack_size;
    exec_env->wasm_stack.s.top = exec_env->wasm_stack.s.bottom;
    return exec_env;
}

void
wasm_exec_env_destroy(WASMExecEnv *exec_env)
{
    wasm_free(exec_env);
}

WASMModuleInstanceCommon *
wasm_exec_env_get_module_inst(WASMExecEnv *exec_env)
{
    return exec_env->module_inst;
}

