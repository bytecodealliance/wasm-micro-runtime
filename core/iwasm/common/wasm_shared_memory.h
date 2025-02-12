/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_SHARED_MEMORY_H
#define _WASM_SHARED_MEMORY_H

#include "bh_common.h"
#include "../interpreter/wasm_runtime.h"
#include "wasm_runtime_common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern korp_mutex g_shared_memory_lock;

bool
wasm_shared_memory_init(void);

void
wasm_shared_memory_destroy(void);

uint16
shared_memory_inc_reference(WASMMemoryInstance *memory);

uint16
shared_memory_dec_reference(WASMMemoryInstance *memory);

#define shared_memory_is_shared(memory) memory->is_shared_memory

#define shared_memory_lock(memory)                                            \
    do {                                                                      \
        /*                                                                    \
         * Note: exception logic is currently abusing this lock.              \
         * cf.                                                                \
         * https://github.com/bytecodealliance/wasm-micro-runtime/issues/2407 \
         */                                                                   \
        bh_assert(memory != NULL);                                            \
        if (memory->is_shared_memory)                                         \
            os_mutex_lock(&g_shared_memory_lock);                             \
    } while (0)

#define shared_memory_unlock(memory)                \
    do {                                            \
        if (memory->is_shared_memory)               \
            os_mutex_unlock(&g_shared_memory_lock); \
    } while (0)

uint32
wasm_runtime_atomic_wait(WASMModuleInstanceCommon *module, void *address,
                         uint64 expect, int64 timeout, bool wait64);

uint32
wasm_runtime_atomic_notify(WASMModuleInstanceCommon *module, void *address,
                           uint32 count);

#if WASM_ENABLE_MULTI_MEMORY != 0
/* Only enable shared heap for the default memory */
#define is_default_memory (memidx == 0)
#else
#define is_default_memory true
#endif
#if WASM_ENABLE_MEMORY64 != 0
#define get_shared_heap_start_off(shared_heap) \
    (is_memory64 ? shared_heap->start_off_mem64 : shared_heap->start_off_mem32)
#else
#define get_shared_heap_start_off(shared_heap) (shared_heap->start_off_mem32)
#endif
/* Check whether the app addr in the last visited shared heap, if not, check the
 * shared heap chain to find which(if any) shared heap the app addr in, and
 * update the last visited shared heap info if found. */
#define app_addr_in_shared_heap(app_addr, bytes)                               \
    (shared_heap && is_default_memory && (app_addr) >= shared_heap_start_off   \
     && (app_addr) <= shared_heap_end_off - bytes + 1)                         \
        || ({                                                                  \
               bool in_chain = false;                                          \
               WASMSharedHeap *cur;                                            \
               uint64 cur_shared_heap_start_off, cur_shared_heap_end_off;      \
               for (cur = shared_heap; cur; cur = cur->chain_next) {           \
                   cur_shared_heap_start_off = get_shared_heap_start_off(cur); \
                   cur_shared_heap_end_off =                                   \
                       cur_shared_heap_start_off - 1 + cur->size;              \
                   if ((app_addr) >= cur_shared_heap_start_off                 \
                       && (app_addr) <= cur_shared_heap_end_off - bytes + 1) { \
                       shared_heap_start_off = cur_shared_heap_start_off;      \
                       shared_heap_end_off = cur_shared_heap_end_off;          \
                       shared_heap_base_addr = cur->base_addr;                 \
                       in_chain = true;                                        \
                       break;                                                  \
                   }                                                           \
               }                                                               \
               in_chain;                                                       \
           })

#define shared_heap_addr_app_to_native(app_addr, native_addr) \
    native_addr = shared_heap_base_addr + ((app_addr)-shared_heap_start_off)

#define CHECK_SHARED_HEAP_OVERFLOW(app_addr, bytes, native_addr) \
    if (app_addr_in_shared_heap(app_addr, bytes))                \
        shared_heap_addr_app_to_native(app_addr, native_addr);   \
    else
#else
#define CHECK_SHARED_HEAP_OVERFLOW(app_addr, bytes, native_addr)

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_SHARED_MEMORY_H */
