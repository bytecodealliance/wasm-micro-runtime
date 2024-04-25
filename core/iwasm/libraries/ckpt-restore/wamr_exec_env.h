/*
 * The WebAssembly Live Migration Project
 *
 *  By: Aibo Hu
 *      Yiwei Yang
 *      Brian Zhao
 *      Andrew Quinn
 *
 *  Copyright 2024 Regents of the Univeristy of California
 *  UC Santa Cruz Sluglab.
 */

#ifndef MVVM_WAMR_EXEC_ENV_H
#define MVVM_WAMR_EXEC_ENV_H
#include "wamr_interp_frame.h"
#include "wamr_module_instance.h"
#include <memory>
#include <ranges>
#include <tuple>
#include <vector>

struct WAMRExecEnv { // multiple
    /** We need to put Module Inst at the top. */
    WAMRModuleInstance module_inst{};
    /* The thread id of wasm interpreter for current thread. */
    uint64 cur_count{};
    uint32 flags{};

    /* Auxiliary stack boundary */
    uint32 aux_boundary{};
    /* Auxiliary stack bottom */
    uint32 aux_bottom{};
    /* Auxiliary stack top */
    std::vector<std::unique_ptr<WAMRInterpFrame>> frames;

    void dump_impl(WASMExecEnv *env);
    void restore_impl(WASMExecEnv *env);
};
#if __has_include(<expected>) && __cplusplus > 202002L
template<SerializerTrait<WASMExecEnv *> T>
void
dump(T t, WASMExecEnv *env)
{
    t->dump_impl(env);
}
template<SerializerTrait<WASMExecEnv *> T>
void
restore(T t, WASMExecEnv *env)
{
    t->restore_impl(env);
};
#else
void
dump(WAMRExecEnv *t, WASMExecEnv *env);
void
restore(WAMRExecEnv *t, WASMExecEnv *env);
#endif
#endif // MVVM_WAMR_EXEC_ENV_H
