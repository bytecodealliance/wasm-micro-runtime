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

bool
wasm_shared_memory_init();

void
wasm_shared_memory_destroy();

uint32
shared_memory_inc_reference(WASMMemoryInstance *memory);

uint32
shared_memory_dec_reference(WASMMemoryInstance *memory);

bool
shared_memory_is_shared(WASMMemoryInstance *memory);

void
shared_memory_lock(WASMMemoryInstance *memory);

void
shared_memory_unlock(WASMMemoryInstance *memory);

uint32
wasm_runtime_atomic_wait(WASMModuleInstanceCommon *module, void *address,
                         uint64 expect, int64 timeout, bool wait64);

uint32
wasm_runtime_atomic_notify(WASMModuleInstanceCommon *module, void *address,
                           uint32 count);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_SHARED_MEMORY_H */
