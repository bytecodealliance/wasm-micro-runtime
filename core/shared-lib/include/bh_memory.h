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

#ifndef _BH_MEMORY_H
#define _BH_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#define BH_KB (1024)
#define BH_MB ((BH_KB)*1024)
#define BH_GB ((BH_MB)*1024)

/**
 * Initialize memory allocator with a pool, the bh_malloc/bh_free function
 * will malloc/free memory from the pool
 *
 * @param mem the pool buffer
 * @param bytes the size bytes of the buffer
 *
 * @return 0 if success, -1 otherwise
 */
int bh_memory_init_with_pool(void *mem, unsigned int bytes);

/**
 * Initialize memory allocator with memory allocator, the bh_malloc/bh_free
 * function will malloc/free memory with the allocator passed
 *
 * @param malloc_func the malloc function
 * @param free_func the free function
 *
 * @return 0 if success, -1 otherwise
 */
int bh_memory_init_with_allocator(void *malloc_func, void *free_func);

/**
 * Destroy memory
 */
void bh_memory_destroy();

/**
 * This function allocates a memory chunk from system
 *
 * @param size bytes need allocate
 *
 * @return the pointer to memory allocated
 */
void* bh_malloc(unsigned int size);

/**
 * This function frees memory chunk
 *
 * @param ptr the pointer to memory need free
 */
void bh_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _BH_MEMORY_H */

