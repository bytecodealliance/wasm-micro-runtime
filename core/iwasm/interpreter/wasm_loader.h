/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#ifndef _WASM_LOADER_H
#define _WASM_LOADER_H

#include "wasm.h"
#include "bh_hashmap.h"
#include "../common/wasm_runtime_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load a WASM module from a specified byte buffer.
 *
 * @param buf the byte buffer which contains the WASM binary data
 * @param size the size of the buffer
 * @param error_buf output of the exception info
 * @param error_buf_size the size of the exception string
 *
 * @return return module loaded, NULL if failed
 */
WASMModule *
wasm_loader_load(uint8 *buf, uint32 size,
#if WASM_ENABLE_MULTI_MODULE != 0
                 bool main_module,
#endif
                 const LoadArgs *args, char *error_buf, uint32 error_buf_size);

/**
 * Load a WASM module from a specified WASM section list.
 *
 * @param section_list the section list which contains each section data
 * @param error_buf output of the exception info
 * @param error_buf_size the size of the exception string
 *
 * @return return WASM module loaded, NULL if failed
 */
WASMModule *
wasm_loader_load_from_sections(WASMSection *section_list, char *error_buf,
                               uint32 error_buf_size);

/**
 * Unload a WASM module.
 *
 * @param module the module to be unloaded
 */
void
wasm_loader_unload(WASMModule *module);

/**
 * Find address of related else opcode and end opcode of opcode block/loop/if
 * according to the start address of opcode.
 *
 * @param module the module to find
 * @param start_addr the next address of opcode block/loop/if
 * @param code_end_addr the end address of function code block
 * @param block_type the type of block, 0/1/2 denotes block/loop/if
 * @param p_else_addr returns the else addr if found
 * @param p_end_addr returns the end addr if found
 * @param error_buf returns the error log for this function
 * @param error_buf_size returns the error log string length
 *
 * @return true if success, false otherwise
 */

bool
wasm_loader_find_block_addr(WASMExecEnv *exec_env, BlockAddr *block_addr_cache,
                            const uint8 *start_addr, const uint8 *code_end_addr,
                            uint8 block_type, uint8 **p_else_addr,
                            uint8 **p_end_addr);

#if WASM_ENABLE_SHARED_HEAP != 0
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
#endif /* end of WASM_ENABLE_SHARED_HEAP != 0 */

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_LOADER_H */
