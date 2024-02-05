/*
 * Copyright (C) 2024 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_memutils.h"

void *
bh_memory_remap_slow(void *old_addr, size_t old_size, size_t new_size)
{
    void *new_memory =
        os_mmap(NULL, new_size, MMAP_PROT_WRITE | MMAP_PROT_READ, 0, -1);
    if (!new_memory) {
        return NULL;
    }
    /*
     * bh_memcpy_s can't be used as it doesn't support values bigger than
     * UINT32_MAX
     */
    memcpy(new_memory, old_addr, new_size < old_size ? new_size : old_size);
    os_munmap(old_addr, old_size);

    return new_memory;
}
