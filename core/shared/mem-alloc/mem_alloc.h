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

void
mem_allocator_destroy(mem_allocator_t allocator);

void *
mem_allocator_malloc(mem_allocator_t allocator, uint32_t size);

void *
mem_allocator_realloc(mem_allocator_t allocator, void *ptr, uint32_t size);

void
mem_allocator_free(mem_allocator_t allocator, void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __MEM_ALLOC_H */

