/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_config.h"
#include "bh_platform.h"
#include "bh_memory.h"
#include "mem_alloc.h"
#include <stdlib.h>

#if BEIHAI_ENABLE_MEMORY_PROFILING != 0
#include "bh_thread.h"

/* Memory profile data of a function */
typedef struct memory_profile {
  struct memory_profile *next;
  const char *function_name;
  const char *file_name;
  int line_in_file;
  int malloc_num;
  int free_num;
  int total_malloc;
  int total_free;
} memory_profile_t;

/* Memory in use which grows when bh_malloc was called
 * and decreases when bh_free was called */
static unsigned int memory_in_use = 0;

/* Memory profile data list */
static memory_profile_t *memory_profiles_list = NULL;

/* Lock of the memory profile list */
static korp_mutex profile_lock;
#endif

#ifndef MALLOC_MEMORY_FROM_SYSTEM

typedef enum Memory_Mode {
    MEMORY_MODE_UNKNOWN = 0,
    MEMORY_MODE_POOL,
    MEMORY_MODE_ALLOCATOR
} Memory_Mode;

static Memory_Mode memory_mode = MEMORY_MODE_UNKNOWN;

static mem_allocator_t pool_allocator = NULL;

static void *(*malloc_func)(unsigned int size) = NULL;
static void (*free_func)(void *ptr) = NULL;

static unsigned int global_pool_size;

int bh_memory_init_with_pool(void *mem, unsigned int bytes)
{
    mem_allocator_t _allocator = mem_allocator_create(mem, bytes);

    if (_allocator) {
        memory_mode = MEMORY_MODE_POOL;
        pool_allocator = _allocator;
#if BEIHAI_ENABLE_MEMORY_PROFILING != 0
        vm_mutex_init(&profile_lock);
#endif
        global_pool_size = bytes;
        return 0;
    }
    bh_printf("Init memory with pool (%p, %u) failed.\n", mem, bytes);
    return -1;
}

int bh_memory_init_with_allocator(void *_malloc_func, void *_free_func)
{
    if (_malloc_func && _free_func && _malloc_func != _free_func) {
        memory_mode = MEMORY_MODE_ALLOCATOR;
        malloc_func = _malloc_func;
        free_func = _free_func;
#if BEIHAI_ENABLE_MEMORY_PROFILING != 0
        vm_mutex_init(&profile_lock);
#endif
        return 0;
    }
    bh_printf("Init memory with allocator (%p, %p) failed.\n", _malloc_func,
            _free_func);
    return -1;
}

void bh_memory_destroy()
{
#if BEIHAI_ENABLE_MEMORY_PROFILING != 0
    vm_mutex_destroy(&profile_lock);
#endif
    if (memory_mode == MEMORY_MODE_POOL)
        mem_allocator_destroy(pool_allocator);
    memory_mode = MEMORY_MODE_UNKNOWN;
}

unsigned bh_memory_pool_size()
{
    if (memory_mode == MEMORY_MODE_POOL)
        return global_pool_size;
    else
        return 1 * BH_GB;
}

void* bh_malloc_internal(unsigned int size)
{
    if (memory_mode == MEMORY_MODE_UNKNOWN) {
        bh_printf("bh_malloc failed: memory hasn't been initialize.\n");
        return NULL;
    } else if (memory_mode == MEMORY_MODE_POOL) {
        return mem_allocator_malloc(pool_allocator, size);
    } else {
        return malloc_func(size);
    }
}

void bh_free_internal(void *ptr)
{
    if (memory_mode == MEMORY_MODE_UNKNOWN) {
        bh_printf("bh_free failed: memory hasn't been initialize.\n");
    } else if (memory_mode == MEMORY_MODE_POOL) {
        mem_allocator_free(pool_allocator, ptr);
    } else {
        free_func(ptr);
    }
}

#if BEIHAI_ENABLE_MEMORY_PROFILING != 0
void* bh_malloc_profile(const char *file,
                        int line,
                        const char *func,
                        unsigned int size)
{
    void *p = bh_malloc_internal(size + 8);

    if (p) {
        memory_profile_t *profile;

        vm_mutex_lock(&profile_lock);

        profile = memory_profiles_list;
        while (profile) {
            if (strcmp(profile->function_name, func) == 0
                && strcmp(profile->file_name, file) == 0) {
                break;
            }
            profile = profile->next;
        }

        if (profile) {
            profile->total_malloc += size;/* TODO: overflow check */
            profile->malloc_num++;
        } else {
            profile = bh_malloc_internal(sizeof(memory_profile_t));
            if (!profile) {
              vm_mutex_unlock(&profile_lock);
              bh_memcpy_s(p, size + 8, &size, sizeof(size));
              return (char *)p + 8;
            }

            memset(profile, 0, sizeof(memory_profile_t));
            profile->file_name = file;
            profile->line_in_file = line;
            profile->function_name = func;
            profile->malloc_num = 1;
            profile->total_malloc = size;
            profile->next = memory_profiles_list;
            memory_profiles_list = profile;
        }

        vm_mutex_unlock(&profile_lock);

        bh_memcpy_s(p, size + 8, &size, sizeof(size));
        memory_in_use += size;

        memory_profile_print(file, line, func, size);

        return (char *)p + 8;
    }

    return NULL;
}

void bh_free_profile(const char *file, int line, const char *func, void *ptr)
{
    unsigned int size = *(unsigned int *)((char *)ptr - 8);
    memory_profile_t *profile;

    bh_free_internal((char *)ptr - 8);

    if (memory_in_use >= size)
        memory_in_use -= size;

    vm_mutex_lock(&profile_lock);

    profile = memory_profiles_list;
    while (profile) {
        if (strcmp(profile->function_name, func) == 0
            && strcmp(profile->file_name, file) == 0) {
            break;
        }
        profile = profile->next;
    }

    if (profile) {
        profile->total_free += size;/* TODO: overflow check */
        profile->free_num++;
    } else {
        profile = bh_malloc_internal(sizeof(memory_profile_t));
        if (!profile) {
            vm_mutex_unlock(&profile_lock);
            return;
        }

        memset(profile, 0, sizeof(memory_profile_t));
        profile->file_name = file;
        profile->line_in_file = line;
        profile->function_name = func;
        profile->free_num = 1;
        profile->total_free = size;
        profile->next = memory_profiles_list;
        memory_profiles_list = profile;
    }

    vm_mutex_unlock(&profile_lock);
}

/**
 * Summarize memory usage and print it out
 * Can use awk to analyze the output like below:
 * awk -F: '{print $2,$4,$6,$8,$9}' OFS="\t" ./out.txt | sort -n -r -k 1
 */
void memory_usage_summarize()
{
    memory_profile_t *profile;

    vm_mutex_lock(&profile_lock);

    profile = memory_profiles_list;
    while (profile) {
        bh_printf("malloc:%d:malloc_num:%d:free:%d:free_num:%d:%s\n",
            profile->total_malloc,
            profile->malloc_num,
            profile->total_free,
            profile->free_num,
            profile->function_name);
        profile = profile->next;
    }

    vm_mutex_unlock(&profile_lock);
}

void memory_profile_print(const char *file,
                          int line,
                          const char *func,
                          int alloc)
{
    bh_printf("location:%s@%d:used:%d:contribution:%d\n",
           func, line, memory_in_use, alloc);
}

#else

void* bh_malloc(unsigned int size)
{
    return bh_malloc_internal(size);
}

void bh_free(void *ptr)
{
    bh_free_internal(ptr);
}
#endif

#else /* else of MALLOC_MEMORY_FROM_SYSTEM */

#if BEIHAI_ENABLE_MEMORY_PROFILING == 0

void* bh_malloc(unsigned int size)
{
    return malloc(size);
}

void bh_free(void *ptr)
{
    if (ptr)
        free(ptr);
}

#else /* else of BEIHAI_ENABLE_MEMORY_PROFILING */

void* bh_malloc_profile(const char *file,
                        int line,
                        const char *func,
                        unsigned int size)
{
    (void)file;
    (void)line;
    (void)func;

    (void)memory_profiles_list;
    (void)profile_lock;
    (void)memory_in_use;

    return malloc(size);
}

void bh_free_profile(const char *file, int line, const char *func, void *ptr)
{
    (void)file;
    (void)line;
    (void)func;

    if (ptr)
        free(ptr);
}
#endif /* end of BEIHAI_ENABLE_MEMORY_PROFILING */
#endif /* end of MALLOC_MEMORY_FROM_SYSTEM*/
