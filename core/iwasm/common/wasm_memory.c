/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_runtime_common.h"
#include "bh_platform.h"
#include "mem_alloc.h"

#if BH_ENABLE_MEMORY_PROFILING != 0

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

/* Memory in use which grows when BH_MALLOC was called
 * and decreases when bh_free was called */
static unsigned int memory_in_use = 0;

/* Memory profile data list */
static memory_profile_t *memory_profiles_list = NULL;

/* Lock of the memory profile list */
static korp_mutex profile_lock;
#endif /* end of BH_ENABLE_MEMORY_PROFILING */

typedef enum Memory_Mode {
    MEMORY_MODE_UNKNOWN = 0,
    MEMORY_MODE_POOL,
    MEMORY_MODE_ALLOCATOR
} Memory_Mode;

static Memory_Mode memory_mode = MEMORY_MODE_UNKNOWN;

static mem_allocator_t pool_allocator = NULL;

static void *(*malloc_func)(unsigned int size) = NULL;
static void *(*realloc_func)(void *ptr, unsigned int size) = NULL;
static void (*free_func)(void *ptr) = NULL;

static unsigned int global_pool_size;

static bool
wasm_memory_init_with_pool(void *mem, unsigned int bytes)
{
    mem_allocator_t _allocator = mem_allocator_create(mem, bytes);

    if (_allocator) {
        memory_mode = MEMORY_MODE_POOL;
        pool_allocator = _allocator;
#if BH_ENABLE_MEMORY_PROFILING != 0
        os_mutex_init(&profile_lock);
#endif
        global_pool_size = bytes;
        return true;
    }
    LOG_ERROR("Init memory with pool (%p, %u) failed.\n", mem, bytes);
    return false;
}

static bool
wasm_memory_init_with_allocator(void *_malloc_func,
                                void *_realloc_func,
                                void *_free_func)
{
    if (_malloc_func && _free_func && _malloc_func != _free_func) {
        memory_mode = MEMORY_MODE_ALLOCATOR;
        malloc_func = _malloc_func;
        realloc_func = _realloc_func;
        free_func = _free_func;
#if BH_ENABLE_MEMORY_PROFILING != 0
        os_mutex_init(&profile_lock);
#endif
        return true;
    }
    LOG_ERROR("Init memory with allocator (%p, %p, %p) failed.\n",
              _malloc_func, _realloc_func, _free_func);
    return false;
}

bool
wasm_runtime_memory_init(mem_alloc_type_t mem_alloc_type,
                         const MemAllocOption *alloc_option)
{
    if (mem_alloc_type == Alloc_With_Pool)
        return wasm_memory_init_with_pool(alloc_option->pool.heap_buf,
                                          alloc_option->pool.heap_size);
    else if (mem_alloc_type == Alloc_With_Allocator)
        return wasm_memory_init_with_allocator(alloc_option->allocator.malloc_func,
                                               alloc_option->allocator.realloc_func,
                                               alloc_option->allocator.free_func);
    else if (mem_alloc_type == Alloc_With_System_Allocator)
        return wasm_memory_init_with_allocator(os_malloc, os_realloc, os_free);
    else
        return false;
}

void
wasm_runtime_memory_destroy()
{
#if BH_ENABLE_MEMORY_PROFILING != 0
    os_mutex_destroy(&profile_lock);
#endif
    if (memory_mode == MEMORY_MODE_POOL)
        mem_allocator_destroy(pool_allocator);
    memory_mode = MEMORY_MODE_UNKNOWN;
}

unsigned
wasm_runtime_memory_pool_size()
{
    if (memory_mode == MEMORY_MODE_POOL)
        return global_pool_size;
    else
        return 1 * BH_GB;
}

static inline void *
wasm_runtime_malloc_internal(unsigned int size)
{
    if (memory_mode == MEMORY_MODE_UNKNOWN) {
        LOG_WARNING("wasm_runtime_malloc failed: memory hasn't been initialize.\n");
        return NULL;
    }
    else if (memory_mode == MEMORY_MODE_POOL) {
        return mem_allocator_malloc(pool_allocator, size);
    }
    else {
        return malloc_func(size);
    }
}

static inline void *
wasm_runtime_realloc_internal(void *ptr, unsigned int size)
{
    if (memory_mode == MEMORY_MODE_UNKNOWN) {
        LOG_WARNING("wasm_runtime_realloc failed: memory hasn't been initialize.\n");
        return NULL;
    }
    else if (memory_mode == MEMORY_MODE_POOL) {
        return mem_allocator_realloc(pool_allocator, ptr, size);
    }
    else {
        if (realloc_func)
            return realloc_func(ptr, size);
        else
            return NULL;
    }
}

static inline void
wasm_runtime_free_internal(void *ptr)
{
    if (memory_mode == MEMORY_MODE_UNKNOWN) {
        LOG_WARNING("wasm_runtime_free failed: memory hasn't been initialize.\n");
    }
    else if (memory_mode == MEMORY_MODE_POOL) {
        mem_allocator_free(pool_allocator, ptr);
    }
    else {
        free_func(ptr);
    }
}

void *
wasm_runtime_malloc(unsigned int size)
{
    return wasm_runtime_malloc_internal(size);
}

void *
wasm_runtime_realloc(void *ptr, unsigned int size)
{
    return wasm_runtime_realloc_internal(ptr, size);
}

void
wasm_runtime_free(void *ptr)
{
    wasm_runtime_free_internal(ptr);
}

#if 0
static uint64 total_malloc = 0;
static uint64 total_free = 0;

void *
wasm_runtime_malloc(unsigned int size)
{
    void *ret = wasm_runtime_malloc_internal(size + 8);

    if (ret) {
        total_malloc += size;
        *(uint32 *)ret = size;
        return (uint8 *)ret + 8;
    }
    else
        return NULL;
}

void *
wasm_runtime_realloc(void *ptr, unsigned int size)
{
    if (!ptr)
        return wasm_runtime_malloc(size);
    else {
        uint8 *ptr_old = (uint8 *)ptr - 8;
        uint32 size_old = *(uint32 *)ptr_old;

        ptr = wasm_runtime_realloc_internal(ptr_old, size + 8);
        if (ptr) {
            total_free += size_old;
            total_malloc += size;
            *(uint32 *)ptr = size;
            return (uint8 *)ptr + 8;
        }
        return NULL;
    }
}

void
wasm_runtime_free(void *ptr)
{
    if (ptr) {
        uint8 *ptr_old = (uint8 *)ptr - 8;
        uint32 size_old = *(uint32 *)ptr_old;

        total_free += size_old;
        wasm_runtime_free_internal(ptr_old);
    }
}

void dump_memory_usage()
{
    os_printf("Memory usage:\n");
    os_printf("    total malloc: %"PRIu64"\n", total_malloc);
    os_printf("    total free: %"PRIu64"\n", total_free);
}
#endif

#if BH_ENABLE_MEMORY_PROFILING != 0
void
memory_profile_print(const char *file, int line,
                     const char *func, int alloc)
{
    os_printf("location:%s@%d:used:%d:contribution:%d\n",
              func, line, memory_in_use, alloc);
}

void *
wasm_runtime_malloc_profile(const char *file, int line,
                            const char *func, unsigned int size)
{
    void *p = wasm_runtime_malloc(size + 8);

    if (p) {
        memory_profile_t *profile;

        os_mutex_lock(&profile_lock);

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
            profile = wasm_runtime_malloc(sizeof(memory_profile_t));
            if (!profile) {
              os_mutex_unlock(&profile_lock);
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

        os_mutex_unlock(&profile_lock);

        bh_memcpy_s(p, size + 8, &size, sizeof(size));
        memory_in_use += size;

        memory_profile_print(file, line, func, size);

        return (char *)p + 8;
    }

    return NULL;
}

void
wasm_runtime_free_profile(const char *file, int line,
                          const char *func, void *ptr)
{
    unsigned int size = *(unsigned int *)((char *)ptr - 8);
    memory_profile_t *profile;

    wasm_runtime_free((char *)ptr - 8);

    if (memory_in_use >= size)
        memory_in_use -= size;

    os_mutex_lock(&profile_lock);

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
        profile = wasm_runtime_malloc(sizeof(memory_profile_t));
        if (!profile) {
            os_mutex_unlock(&profile_lock);
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

    os_mutex_unlock(&profile_lock);
}

/**
 * Summarize memory usage and print it out
 * Can use awk to analyze the output like below:
 * awk -F: '{print $2,$4,$6,$8,$9}' OFS="\t" ./out.txt | sort -n -r -k 1
 */
void memory_usage_summarize()
{
    memory_profile_t *profile;

    os_mutex_lock(&profile_lock);

    profile = memory_profiles_list;
    while (profile) {
        os_printf("malloc:%d:malloc_num:%d:free:%d:free_num:%d:%s\n",
                  profile->total_malloc,
                  profile->malloc_num,
                  profile->total_free,
                  profile->free_num,
                  profile->function_name);
        profile = profile->next;
    }

    os_mutex_unlock(&profile_lock);
}
#endif /* end of BH_ENABLE_MEMORY_PROFILING */

