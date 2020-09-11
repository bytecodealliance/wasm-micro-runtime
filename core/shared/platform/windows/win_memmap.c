/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"

void * os_mmap(void *hint, size_t size, int prot, int flags)
{
    DWORD AllocType = MEM_RESERVE | MEM_COMMIT;
    DWORD flProtect = PAGE_NOACCESS;
    size_t request_size, page_size;
    void *addr;

    page_size = getpagesize();
    request_size = (size + page_size - 1) & ~(page_size - 1);

    if (request_size < size)
        /* integer overflow */
        return NULL;

    if (request_size == 0)
        request_size = page_size;

    if (prot & MMAP_PROT_EXEC) {
        if (prot & MMAP_PROT_WRITE)
            flProtect = PAGE_EXECUTE_READWRITE;
        else
            flProtect = PAGE_EXECUTE_READ;
    }
    else if (prot & MMAP_PROT_WRITE)
        flProtect = PAGE_READWRITE;
    else if (prot & MMAP_PROT_READ)
        flProtect = PAGE_READONLY;


    addr = VirtualAlloc((LPVOID)hint, request_size, AllocType,
                        flProtect);
    return addr;
}

void
os_munmap(void *addr, size_t size)
{
    size_t page_size = getpagesize();
    size_t request_size = (size + page_size - 1) & ~(page_size - 1);

    if (addr) {
        if (VirtualFree(addr, 0, MEM_RELEASE) == 0) {
            if (VirtualFree(addr, size, MEM_DECOMMIT) == 0) {
                os_printf("os_munmap error addr:%p, size:0x%lx, errno:%d\n",
                          addr, request_size, errno);
            }
        }
    }
}

int
os_mprotect(void *addr, size_t size, int prot)
{
    DWORD AllocType = MEM_RESERVE | MEM_COMMIT;
    DWORD flProtect = PAGE_NOACCESS;

    if (!addr)
        return 0;

    if (prot & MMAP_PROT_EXEC) {
        if (prot & MMAP_PROT_WRITE)
            flProtect = PAGE_EXECUTE_READWRITE;
        else
            flProtect = PAGE_EXECUTE_READ;
    }
    else if (prot & MMAP_PROT_WRITE)
        flProtect = PAGE_READWRITE;
    else if (prot & MMAP_PROT_READ)
        flProtect = PAGE_READONLY;

    return VirtualProtect((LPVOID)addr, size, flProtect, NULL);
}

void
os_dcache_flush(void)
{

}
