/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MEM_ALLOC_H
#define __MEM_ALLOC_H

#include <inttypes.h>

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

void
mem_allocator_free(mem_allocator_t allocator, void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __MEM_ALLOC_H */

