/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"

void *
os_mmap(void *hint, size_t size, int prot, int flags)
{
    int map_prot = PROT_NONE;
    int map_flags = MAP_ANONYMOUS | MAP_PRIVATE;
    uint64 request_size, page_size;
    uint8 *addr;
    uint32 i;

    page_size = (uint64)getpagesize();
    request_size = (size + page_size - 1) & ~(page_size - 1);

    if ((size_t)request_size < size)
        /* integer overflow */
        return NULL;

    if (request_size > 16 * (uint64)UINT32_MAX)
        /* At most 16 G is allowed */
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

    return addr;
}

void
os_munmap(void *addr, size_t size)
{
    uint64 page_size = (uint64)getpagesize();
    uint64 request_size = (size + page_size - 1) & ~(page_size - 1);

    if (addr) {
        if (munmap(addr, request_size)) {
            os_printf("os_munmap error addr:%p, size:0x%"PRIx64", errno:%d\n",
                      addr, request_size, errno);
        }
    }
}

int
os_mprotect(void *addr, size_t size, int prot)
{
    int map_prot = PROT_NONE;
    uint64 page_size = (uint64)getpagesize();
    uint64 request_size = (size + page_size - 1) & ~(page_size - 1);

    if (!addr)
        return 0;

    if (prot & MMAP_PROT_READ)
        map_prot |= PROT_READ;

    if (prot & MMAP_PROT_WRITE)
        map_prot |= PROT_WRITE;

    if (prot & MMAP_PROT_EXEC)
        map_prot |= PROT_EXEC;

    return mprotect(addr, request_size, map_prot);
}

void
os_dcache_flush(void)
{
}
