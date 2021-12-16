/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"
#include "platform_api_extension.h"

void *
os_mmap(void *hint, size_t size, int prot, int flags)
{
    return os_malloc((int)size);
}

void
os_munmap(void *addr, size_t size)
{
    return os_free(addr);
}

int
os_mprotect(void *addr, size_t size, int prot)
{
    return 0;
}

void
os_dcache_flush()
{}
