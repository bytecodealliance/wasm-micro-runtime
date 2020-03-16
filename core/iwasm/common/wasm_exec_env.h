/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_EXEC_ENV_H
#define _WASM_EXEC_ENV_H

#include "bh_thread.h"
#include "bh_assert.h"
#if WASM_ENABLE_INTERP != 0
#include "../interpreter/wasm.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct WASMModuleInstanceCommon;
struct WASMInterpFrame;

/* Execution environment */
typedef struct WASMExecEnv {
    /* Next thread's exec env of a WASM module instance.  */
    struct WASMExecEnv *next;

    /* Previous thread's exec env of a WASM module instance.  */
    struct WASMExecEnv *prev;

    /* The WASM module instance of current thread */
    struct WASMModuleInstanceCommon *module_inst;

#if WASM_ENABLE_AOT != 0
    uint32 *argv_buf;
#endif

    /* Current interpreter frame of current thread */
    struct WASMInterpFrame *cur_frame;

    /* The native thread handle of current thread */
    korp_tid handle;

#if WASM_ENABLE_INTERP != 0
    BlockAddr block_addr_cache[BLOCK_ADDR_CACHE_SIZE][BLOCK_ADDR_CONFLICT_SIZE];
#endif

    /* The boundary of native stack. When interpreter detects that native
       frame may overrun this boundary, it throws a stack overflow
       exception. */
    void *native_stack_boundary;

    /* The WASM stack size */
    uint32 wasm_stack_size;

    /* The WASM stack of current thread */
    union {
        uint64 __make_it_8_byte_aligned_;

        struct {
            /* The top boundary of the stack.  */
            uint8 *top_boundary;

            /* Top cell index which is free.  */
            uint8 *top;

            /* The Java stack.  */
            uint8 bottom[1];
        } s;
    } wasm_stack;
} WASMExecEnv;

WASMExecEnv *
wasm_exec_env_create(struct WASMModuleInstanceCommon *module_inst,
                     uint32 stack_size);

void
wasm_exec_env_destroy(WASMExecEnv *exec_env);

/**
 * Allocate a WASM frame from the WASM stack.
 *
 * @param exec_env the current execution environment
 * @param size size of the WASM frame, it must be a multiple of 4
 *
 * @return the WASM frame if there is enough space in the stack area
 * with a protection area, NULL otherwise
 */
static inline void *
wasm_exec_env_alloc_wasm_frame(WASMExecEnv *exec_env, unsigned size)
{
    uint8 *addr = exec_env->wasm_stack.s.top;

    bh_assert(!(size & 3));

    /* The outs area size cannot be larger than the frame size, so
       multiplying by 2 is enough. */
    if (addr + size * 2 > exec_env->wasm_stack.s.top_boundary) {
        /* WASM stack overflow. */
        /* When throwing SOE, the preserved space must be enough. */
        /* bh_assert(!exec_env->throwing_soe);*/
        return NULL;
    }

    exec_env->wasm_stack.s.top += size;

    return addr;
}

static inline void
wasm_exec_env_free_wasm_frame(WASMExecEnv *exec_env, void *prev_top)
{
    bh_assert((uint8 *)prev_top >= exec_env->wasm_stack.s.bottom);
    exec_env->wasm_stack.s.top = (uint8 *)prev_top;
}

/**
 * Get the current WASM stack top pointer.
 *
 * @param exec_env the current execution environment
 *
 * @return the current WASM stack top pointer
 */
static inline void*
wasm_exec_env_wasm_stack_top(WASMExecEnv *exec_env)
{
    return exec_env->wasm_stack.s.top;
}

/**
 * Set the current frame pointer.
 *
 * @param exec_env the current execution environment
 * @param frame the WASM frame to be set for the current exec env
 */
static inline void
wasm_exec_env_set_cur_frame(WASMExecEnv *exec_env,
                            struct WASMInterpFrame *frame)
{
    exec_env->cur_frame = frame;
}

/**
 * Get the current frame pointer.
 *
 * @param exec_env the current execution environment
 *
 * @return the current frame pointer
 */
static inline struct WASMInterpFrame*
wasm_exec_env_get_cur_frame(WASMExecEnv *exec_env)
{
    return exec_env->cur_frame;
}

struct WASMModuleInstanceCommon *
wasm_exec_env_get_module_inst(WASMExecEnv *exec_env);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_EXEC_ENV_H */
