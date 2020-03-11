/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "bh_common.h"
#include <stdlib.h>
#include <string.h>

int
bh_platform_init()
{
    return 0;
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
bh_mmap(void *hint, unsigned int size, int prot, int flags)
{
    return BH_MALLOC(size);
}

void
bh_munmap(void *addr, uint32 size)
{
    return BH_FREE(addr);
}

int
bh_mprotect(void *addr, uint32 size, int prot)
{
    return 0;
}

void
bh_dcache_flush()
{
}
