/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"

void *
os_mmap(void *hint, uint32 size, int prot, int flags)
{
    int map_prot = PROT_NONE;
    int map_flags = MAP_ANONYMOUS | MAP_PRIVATE;
    uint64 request_size, page_size;
    uint8 *addr, *addr_aligned;
    uint32 i;

    /* align to 2M if no less than 2M, else align to 4K */
    page_size = size < 2 * 1024 * 1024 ? 4096 : 2 * 1024 * 1024;
    request_size = (size + page_size - 1) & ~(page_size - 1);
    request_size += page_size;

    if (request_size >= UINT32_MAX)
        return NULL;

    if (prot & MMAP_PROT_READ)
        map_prot |= PROT_READ;

    if (prot & MMAP_PROT_WRITE)
        map_prot |= PROT_WRITE;

    if (prot & MMAP_PROT_EXEC)
        map_prot |= PROT_EXEC;

#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
#ifndef __APPLE__
    if (flags & MMAP_MAP_32BIT)
        map_flags |= MAP_32BIT;
#endif
#endif

    if (flags & MMAP_MAP_FIXED)
        map_flags |= MAP_FIXED;

    /* try 5 times */
    for (i = 0; i < 5; i ++) {
        addr = mmap(hint, request_size, map_prot, map_flags, -1, 0);
        if (addr != MAP_FAILED)
            break;
    }
    if (addr == MAP_FAILED)
        return NULL;

    addr_aligned = (uint8*)(uintptr_t)
        (((uint64)(uintptr_t)addr + page_size - 1) & ~(page_size - 1));

    /* Unmap memory allocated before the aligned base address */
    if (addr != addr_aligned) {
        uint32 prefix_size = (uint32)(addr_aligned - addr);
        munmap(addr, prefix_size);
        request_size -= prefix_size;
    }

    /* Unmap memory allocated after the potentially unaligned end */
    if (size != request_size) {
        uint32 suffix_size = (uint32)(request_size - size);
        munmap(addr_aligned + size, suffix_size);
        request_size -= size;
    }

#ifndef __APPLE__
    if (size >= 2 * 1024 * 1024) {
        /* Try to use huge page to improve performance */
        if (!madvise(addr, size, MADV_HUGEPAGE))
            /* make huge page become effective */
            memset(addr, 0, size);
    }
#endif

    return addr_aligned;
}

void
os_munmap(void *addr, uint32 size)
{
    if (addr)
        munmap(addr, size);
}

int
os_mprotect(void *addr, uint32 size, int prot)
{
    int map_prot = PROT_NONE;

    if (!addr)
        return 0;

    if (prot & MMAP_PROT_READ)
        map_prot |= PROT_READ;

    if (prot & MMAP_PROT_WRITE)
        map_prot |= PROT_WRITE;

    if (prot & MMAP_PROT_EXEC)
        map_prot |= PROT_EXEC;

    return mprotect(addr, size, map_prot);
}

void
os_dcache_flush(void)
{
}
