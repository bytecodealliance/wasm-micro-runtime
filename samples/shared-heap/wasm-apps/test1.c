/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdint.h>

extern void *
shared_malloc(uint32_t size);
extern void
shared_free(void *ptr);

void *
my_shared_malloc(uint32_t size, uint32_t index)
{
    char *buf = shared_malloc(size);

    if (buf)
        snprintf(buf, 1024, "Hello, this is buf %u allocated from shared heap",
                 index);

    return buf;
}

void
my_shared_free(void *ptr)
{
    shared_free(ptr);
}
