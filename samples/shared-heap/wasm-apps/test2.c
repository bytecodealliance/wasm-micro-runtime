/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdint.h>

extern void
shared_heap_free(void *ptr);

void
print_buf(char *buf)
{
    printf("wasm app2's wasm func received buf: %s\n\n", buf);
    shared_heap_free(buf);
}

void
consume_str(char *buf)
{
    printf("wasm app2's wasm func received buf: %s\n\n", buf);
}
