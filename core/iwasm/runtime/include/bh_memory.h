/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
 * Get the pool size of memory, if memory is initialized with allocator,
 * return 1GB by default.
 */
int bh_memory_pool_size();

#if BEIHAI_ENABLE_MEMORY_PROFILING == 0

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

#else

void* bh_malloc_profile(const char *file, int line, const char *func, unsigned int size);
void bh_free_profile(const char *file, int line, const char *func, void *ptr);

#define bh_malloc(size) bh_malloc_profile(__FILE__, __LINE__, __func__, size)
#define bh_free(ptr) bh_free_profile(__FILE__, __LINE__, __func__, ptr)

/**
 * Print current memory profiling data
 *
 * @param file file name of the caller
 * @param line line of the file of the caller
 * @param func function name of the caller
 */
void memory_profile_print(const char *file, int line, const char *func, int alloc);

/**
 * Summarize memory usage and print it out
 * Can use awk to analyze the output like below:
 * awk -F: '{print $2,$4,$6,$8,$9}' OFS="\t" ./out.txt | sort -n -r -k 1
 */
void memory_usage_summarize();

#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _BH_MEMORY_H */

