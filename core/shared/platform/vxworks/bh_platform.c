/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "bh_common.h"
#include "bh_assert.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>

int
bh_platform_init()
{
    return 0;
}

char*
bh_read_file_to_buffer(const char *filename, uint32 *ret_size)
{
    char *buffer;
    int file;
    uint32 file_size, read_size;
    struct stat stat_buf;

    if (!filename || !ret_size) {
        printf("Read file to buffer failed: invalid filename or ret size.\n");
        return NULL;
    }

    if ((file = open(filename, O_RDONLY, 0)) == -1) {
        printf("Read file to buffer failed: open file %s failed.\n",
               filename);
        return NULL;
    }

    if (fstat(file, &stat_buf) != 0) {
        printf("Read file to buffer failed: fstat file %s failed.\n",
               filename);
        close(file);
        return NULL;
    }

    file_size = (uint32)stat_buf.st_size;

    if (!(buffer = bh_malloc(file_size))) {
        printf("Read file to buffer failed: alloc memory failed.\n");
        close(file);
        return NULL;
    }

    read_size = (uint32)read(file, buffer, file_size);
    close(file);

    if (read_size < file_size) {
        printf("Read file to buffer failed: read file content failed.\n");
        bh_free(buffer);
        return NULL;
    }

    *ret_size = file_size;
    return buffer;
}

void *
bh_mmap(void *hint, uint32 size, int prot, int flags)
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
    if (flags & MMAP_MAP_32BIT)
        map_flags |= MAP_32BIT;
#endif

    if (flags & MMAP_MAP_FIXED)
        map_flags |= MAP_FIXED;

    /* try 5 times */
    for (i = 0; i < 5; i ++) {
        addr = mmap(hint, size, map_prot, map_flags, -1, 0);
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

    if (size >= 2 * 1024 * 1024) {
        /* Try to use huge page to improve performance */
        if (!madvise(addr, size, MADV_HUGEPAGE))
            /* make huge page become effective */
            memset(addr, 0, size);
    }

    return addr_aligned;
}

void
bh_munmap(void *addr, uint32 size)
{
    if (addr)
        munmap(addr, size);
}

int
bh_mprotect(void *addr, uint32 size, int prot)
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

