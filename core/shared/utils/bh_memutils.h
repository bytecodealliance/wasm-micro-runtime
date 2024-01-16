/*
 * Copyright (C) 2024 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _BH_MEMUTILS_H
#define _BH_MEMUTILS_H

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Remaps memory by mapping a new region, copying data from the old
 * region and umapping the old region.
 *
 * Unless the behavior is desired, in most cases os_mremap should be used
 * as it's at worst equally slow as this function, and on some platforms
 * (e.g. posix with mremap) os_mremap will perform better.
 *
 * @param old_addr an old address.
 * @param old_size a size of the old address.
 * @param new_size a size of the new memory region.
 * @return a pointer to the new memory region.
 */
void *
bh_memory_remap_slow(void *old_addr, size_t old_size, size_t new_size);

#ifdef __cplusplus
}
#endif

#endif /* end of _BH_MEMUTILS_H */
