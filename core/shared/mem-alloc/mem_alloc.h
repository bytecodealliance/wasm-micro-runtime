/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef __MEM_ALLOC_H
#define __MEM_ALLOC_H

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *mem_allocator_t;

mem_allocator_t
mem_allocator_create(void *mem, uint32_t size);

mem_allocator_t
mem_allocator_create_with_struct_and_pool(void *struct_buf,
                                          uint32_t struct_buf_size,
                                          void *pool_buf,
                                          uint32_t pool_buf_size);

void
mem_allocator_destroy(mem_allocator_t allocator);

uint32
mem_allocator_get_heap_struct_size(void);

void *
mem_allocator_malloc(mem_allocator_t allocator, uint32_t size);

void *
mem_allocator_realloc(mem_allocator_t allocator, void *ptr, uint32_t size);

void
mem_allocator_free(mem_allocator_t allocator, void *ptr);

int
mem_allocator_migrate(mem_allocator_t allocator, char *pool_buf_new,
                      uint32 pool_buf_size);

bool
mem_allocator_is_heap_corrupted(mem_allocator_t allocator);

#if WASM_ENABLE_GC != 0
void *
mem_allocator_malloc_with_gc(mem_allocator_t allocator, uint32_t size);

#if WASM_GC_MANUALLY != 0
void
mem_allocator_free_with_gc(mem_allocator_t allocator, void *ptr);

#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __MEM_ALLOC_H */
