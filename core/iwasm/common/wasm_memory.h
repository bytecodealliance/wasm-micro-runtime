/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_MEMORY_H
#define _WASM_MEMORY_H

#include "bh_common.h"
#include "../include/wasm_export.h"
#include "../interpreter/wasm_runtime.h"
#include "../common/wasm_shared_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#if WASM_ENABLE_SHARED_MEMORY != 0 && BH_ATOMIC_64_IS_ATOMIC != 0
#define GET_LINEAR_MEMORY_SIZE(memory) \
    BH_ATOMIC_64_LOAD(memory->memory_data_size)
#define SET_LINEAR_MEMORY_SIZE(memory, size) \
    BH_ATOMIC_64_STORE(memory->memory_data_size, size)
#elif WASM_ENABLE_SHARED_MEMORY != 0
static inline uint64
GET_LINEAR_MEMORY_SIZE(const WASMMemoryInstance *memory)
{
    SHARED_MEMORY_LOCK(memory);
    uint64 memory_data_size = BH_ATOMIC_64_LOAD(memory->memory_data_size);
    SHARED_MEMORY_UNLOCK(memory);
    return memory_data_size;
}
static inline void
SET_LINEAR_MEMORY_SIZE(WASMMemoryInstance *memory, uint64 size)
{
    SHARED_MEMORY_LOCK(memory);
    BH_ATOMIC_64_STORE(memory->memory_data_size, size);
    SHARED_MEMORY_UNLOCK(memory);
}
#else
#define GET_LINEAR_MEMORY_SIZE(memory) memory->memory_data_size
#define SET_LINEAR_MEMORY_SIZE(memory, size) memory->memory_data_size = size
#endif

bool
wasm_runtime_memory_init(mem_alloc_type_t mem_alloc_type,
                         const MemAllocOption *alloc_option);

void
wasm_runtime_memory_destroy();

unsigned
wasm_runtime_memory_pool_size();

void
wasm_runtime_set_mem_bound_check_bytes(WASMMemoryInstance *memory,
                                       uint64 memory_data_size);

void
wasm_runtime_set_enlarge_mem_error_callback(
    const enlarge_memory_error_callback_t callback, void *user_data);

void
wasm_deallocate_linear_memory(WASMMemoryInstance *memory_inst);

int
wasm_allocate_linear_memory(uint8 **data, bool is_shared_memory,
                            uint64 num_bytes_per_page, uint64 init_page_count,
                            uint64 max_page_count, uint64 *memory_data_size);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_MEMORY_H */
