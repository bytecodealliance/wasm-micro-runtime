/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

extern void *
shared_heap_malloc(uint32_t size);
extern void
shared_heap_free(void *ptr);

void *
my_shared_heap_malloc(uint32_t size, uint32_t index)
{
    char *buf1 = NULL, *buf2 = NULL, *buf;

    char *buf3 = NULL;
    buf3 = malloc(2048);

    buf1 = shared_heap_malloc(1024);
    if (!buf1)
        return NULL;

    buf1[0] = 'H';
    buf1[1] = 'e';
    buf1[2] = 'l';
    buf1[3] = 'l';
    buf1[4] = 'o';
    buf1[5] = ',';
    buf1[6] = ' ';

    buf2 = shared_heap_malloc(1024);
    if (!buf2) {
        shared_heap_free(buf1);
        return NULL;
    }

    snprintf(buf2, 1024, "this is buf %u allocated from shared heap", index);

    buf = shared_heap_malloc(size);
    if (!buf) {
        shared_heap_free(buf1);
        shared_heap_free(buf2);
        return NULL;
    }

    memset(buf, 0, size);
    memcpy(buf, buf1, strlen(buf1));
    memcpy(buf + strlen(buf1), buf2, strlen(buf2));

    uint32_t max_value = UINT32_MAX;
    char *address = (char *)max_value;
    printf("The address of buf1, buf2, buf, UINT32_MAX is: %p,%p,%p,%p\n", buf1,
           buf2, buf, address);
    address[0] = 'c';
    printf("The content address of UINT32_MAX is: %c\n", *address);

    shared_heap_free(buf1);
    shared_heap_free(buf2);
    return buf;
}

void
my_shared_heap_free(void *ptr)
{
    shared_heap_free(ptr);
}
