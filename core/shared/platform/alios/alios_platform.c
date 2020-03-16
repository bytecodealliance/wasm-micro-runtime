/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"
#include "platform_api_extension.h"

int
os_thread_sys_init();

void
os_thread_sys_destroy();

int
bh_platform_init()
{
    return os_thread_sys_init();
}

void
bh_platform_destroy()
{
    os_thread_sys_destroy();
}

void *
os_malloc(unsigned size)
{
    return NULL;
}

void *
os_realloc(void *ptr, unsigned size)
{
    return NULL;
}

void
os_free(void *ptr)
{
}

void *
os_mmap(void *hint, unsigned int size, int prot, int flags)
{
    return BH_MALLOC(size);
}

void
os_munmap(void *addr, uint32 size)
{
    return BH_FREE(addr);
}

int
os_mprotect(void *addr, uint32 size, int prot)
{
    return 0;
}

void
os_dcache_flush()
{
}
