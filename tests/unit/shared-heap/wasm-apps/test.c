/*
 * Copyright (C) 2024 Xiaomi Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>

extern void *
shared_heap_malloc(int size);
extern void
shared_heap_free(void *offset);

int
test()
{
    int *ptr = (int *)shared_heap_malloc(4);

    *ptr = 10;
    int a = *ptr;
    shared_heap_free(ptr);
    return a;
}

int
test_malloc_fail()
{
    int *ptr = (int *)shared_heap_malloc(8192);

    if (ptr == NULL) {
        return 1;
    }
    shared_heap_free(ptr);
    return 0;
}
