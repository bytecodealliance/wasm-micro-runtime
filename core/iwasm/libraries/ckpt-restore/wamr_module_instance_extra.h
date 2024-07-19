/*
 * Regents of the Univeristy of California, All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef MVVM_WAMR_MODULE_INSTANCE_EXTRA_H
#define MVVM_WAMR_MODULE_INSTANCE_EXTRA_H

#if WASM_ENABLE_WASI_NN != 0
#include "wamr_wasi_nn_context.h"
#endif
#include "wamr_serializer.h"
#include "wasm_runtime.h"
#include <memory>
#include <vector>
struct WAMRGlobalInstance {
    uint8 type;
    /* mutable or constant */
    bool is_mutable;
    /* data offset to base_addr of WASMMemoryInstance */
    uint32 data_offset;
    /* initial value */
    uint8 initial_value[4];
    void dump_impl(WASMGlobalInstance *env)
    {
        type = env->type;
        is_mutable = env->is_mutable;
        data_offset = env->data_offset;
        memcpy(initial_value, &env->initial_value, 4);
    };
    void restore_impl(WASMGlobalInstance *env)
    {
        env->type = type;
        env->is_mutable = is_mutable;
        env->data_offset = data_offset;
        memcpy(&env->initial_value, initial_value, 4);
    };
};

template<SerializerTrait<WASMGlobalInstance *> T>
void
dump(T t, WASMGlobalInstance *env);

template<SerializerTrait<WASMGlobalInstance *> T>
void
restore(T t, WASMGlobalInstance *env);

struct WAMRModuleInstanceExtra {
    uint32 global_count{};
    uint32 function_count{};
    std::vector<WAMRGlobalInstance> globals;
#if WASM_ENABLE_WASI_NN != 0
    WAMRWASINNContext wasi_nn_ctx{};
#endif
    void dump_impl(WASMModuleInstanceExtra *env)
    {
        global_count = env->global_count;
        function_count = env->function_count;
        for (int i = 0; i < global_count; i++) {
            dump(&globals[i], env->globals[i]);
        }
#if WASM_ENABLE_WASI_NN != 0
        dump(&wasi_nn_ctx, env->wasi_nn_ctx);
#endif
    };
    void restore_impl(WASMModuleInstanceExtra *env)
    {
        env->global_count = global_count;
        env->function_count = function_count;
        for (int i = 0; i < global_count; i++) {
            restore(&globals[i], env->globals[i]);
        }
#if WASM_ENABLE_WASI_NN != 0
        restore(&wasi_nn_ctx, env->wasi_nn_ctx);
#endif
    };
};
#if __has_include(<expected>) && __cplusplus > 202002L
template<SerializerTrait<WASMModuleInstanceExtra *> T>
void
dump(T t, WASMModuleInstanceExtra *env)
{
    t->dump_impl(env);
}

template<SerializerTrait<WASMModuleInstanceExtra *> T>
void
restore(T t, WASMModuleInstanceExtra *env)
{
    t->restore_impl(env);
}
#else
void
dump(WAMRModuleInstanceExtra *t, WASMModuleInstanceExtra *env);
void
restore(WAMRModuleInstanceExtra *t, WASMModuleInstanceExtra *env);
#endif
#endif // MVVM_WAMR_MODULE_INSTANCE_EXTRA_H
