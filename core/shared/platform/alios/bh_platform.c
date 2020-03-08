/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "bh_memory.h"
#include "bh_common.h"
#include <stdlib.h>
#include <string.h>

int
bh_platform_init()
{
    return 0;
}

void *
bh_mmap(void *hint, unsigned int size, int prot, int flags)
{
    return bh_malloc(size);
}

void
bh_munmap(void *addr, uint32 size)
{
    return bh_free(addr);
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
