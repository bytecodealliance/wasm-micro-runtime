/*
 * Copyright (C) 2024 Xiaomi Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>

extern void *
shared_malloc(int size);
extern void
shared_free(void *offset);

int
test()
{
    int *ptr = (int *)shared_malloc(10);

    *ptr = 10;
    int a = *ptr;
    shared_free(ptr);
    return a;
}