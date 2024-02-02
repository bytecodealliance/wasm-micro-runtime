/*
 * Copyright (C) 2024 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_memutils.h"

void *
os_mremap(void *old_addr, size_t old_size, size_t new_size)
{
    return bh_memory_remap_slow(old_addr, old_size, new_size);
}
