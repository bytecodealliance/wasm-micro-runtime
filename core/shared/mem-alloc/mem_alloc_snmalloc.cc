/*
 * Copyright (C) 2020 Microsoft.  All rights reserved.
 */

#include "mem_alloc.h"

#define SNMALLOC_MEMORY_PROVIDER PALFixedRegion
#define USE_RESERVE_MULTIPLE 1
#define NO_BOOTSTRAP_ALLOCATOR
#define IS_ADDRESS_SPACE_CONSTRAINED
#define SNMALLOC_EXPOSE_PAGEMAP
#define SNMALLOC_NAME_MANGLE(a) wamr_sn_##a
// Redefine the namespace, so we can a different allocator from the system one.
#define snmalloc snmalloc_wamr
#include "../../../snmalloc/src/override/malloc.cc"

extern "C" {

mem_allocator_t mem_allocator_create(void *mem, uint32_t size)
{
    static bool called = false;
    if (called)
    {
        fprintf(stderr, "snmalloc creation called a second time with argument %p %d", mem, size);
        abort();
    }
    snmalloc_wamr::PALFixedRegion::setup_initial_range(mem, (char*)mem + size);
    called = true;
    return (mem_allocator_t)1;
}

void mem_allocator_destroy(mem_allocator_t allocator)
{
    UNUSED(allocator);
    snmalloc_wamr::current_alloc_pool()->cleanup_unused();
    snmalloc_wamr::current_alloc_pool()->debug_check_empty();
    return;
}

void *
mem_allocator_malloc(mem_allocator_t allocator, uint32_t size)
{
    UNUSED(allocator);
    return wamr_sn_malloc(size);
}

void *
mem_allocator_realloc(mem_allocator_t allocator, void *ptr, uint32_t size)
{
    UNUSED(allocator);
    return wamr_sn_realloc(ptr, size);
}

void mem_allocator_free(mem_allocator_t allocator, void *ptr)
{
    UNUSED(allocator);
    if (ptr)
    {
        wamr_sn_free(ptr);
    }
}

int
mem_allocator_migrate(mem_allocator_t allocator,
                      mem_allocator_t allocator_old)
{
    UNUSED(allocator);
    UNUSED(allocator_old);
    fprintf(stderr, "snmalloc migration not supported");
    abort();
}

int
mem_allocator_reinit_lock(mem_allocator_t allocator)
{
    UNUSED(allocator);
    return 0;
}

void
mem_allocator_destroy_lock(mem_allocator_t allocator)
{
    UNUSED(allocator);
    return;
}

}
