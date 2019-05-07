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

#include "bh_memory.h"
#include "mem_alloc.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef MALLOC_MEMORY_FROM_SYSTEM

typedef enum Memory_Mode {
    MEMORY_MODE_UNKNOWN = 0, MEMORY_MODE_POOL, MEMORY_MODE_ALLOCATOR
} Memory_Mode;

static Memory_Mode memory_mode = MEMORY_MODE_UNKNOWN;

static mem_allocator_t pool_allocator = NULL;

static void *(*malloc_func)(unsigned int size) = NULL;
static void (*free_func)(void *ptr) = NULL;

int bh_memory_init_with_pool(void *mem, unsigned int bytes)
{
    mem_allocator_t _allocator = mem_allocator_create(mem, bytes);

    if (_allocator) {
        memory_mode = MEMORY_MODE_POOL;
        pool_allocator = _allocator;
        return 0;
    }
    printf("Init memory with pool (%p, %u) failed.\n", mem, bytes);
    return -1;
}

int bh_memory_init_with_allocator(void *_malloc_func, void *_free_func)
{
    if (_malloc_func && _free_func && _malloc_func != _free_func) {
        memory_mode = MEMORY_MODE_ALLOCATOR;
        malloc_func = _malloc_func;
        free_func = _free_func;
        return 0;
    }
    printf("Init memory with allocator (%p, %p) failed.\n", _malloc_func,
            _free_func);
    return -1;
}

void bh_memory_destroy()
{
    if (memory_mode == MEMORY_MODE_POOL)
        mem_allocator_destroy(pool_allocator);
    memory_mode = MEMORY_MODE_UNKNOWN;
}

void* bh_malloc(unsigned int size)
{
    if (memory_mode == MEMORY_MODE_UNKNOWN) {
        printf("bh_malloc failed: memory hasn't been initialize.\n");
        return NULL;
    } else if (memory_mode == MEMORY_MODE_POOL) {
        return mem_allocator_malloc(pool_allocator, size);
    } else {
        return malloc_func(size);
    }
}

void bh_free(void *ptr)
{
    if (memory_mode == MEMORY_MODE_UNKNOWN) {
        printf("bh_free failed: memory hasn't been initialize.\n");
    } else if (memory_mode == MEMORY_MODE_POOL) {
        mem_allocator_free(pool_allocator, ptr);
    } else {
        free_func(ptr);
    }
}

#else /* else of MALLOC_MEMORY_FROM_SYSTEM */

void* bh_malloc(unsigned int size)
{
    return malloc(size);
}

void bh_free(void *ptr)
{
    if (ptr)
    free(ptr);
}

#endif /* end of MALLOC_MEMORY_FROM_SYSTEM*/

