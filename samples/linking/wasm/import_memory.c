/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Buffer {
    int size;
    char *data;
};

struct Buffer *
new_buffer(int size)
{
    struct Buffer *buffer = calloc(1, sizeof(struct Buffer));
    if (!buffer) {
        return NULL;
    }

    buffer->size = size;
    buffer->data = calloc(1, size);
    if (!buffer->data) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

void
release_buffer(struct Buffer *buffer)
{
    buffer->size = 0;
    free(buffer->data);
    free(buffer);
}

__attribute__((export_name("goodhart_law"))) void
goodhart_law()
{
    struct Buffer *buffer = new_buffer(64);
    assert(buffer);

    snprintf(buffer->data, 60, "%s",
             "When a measure becomes a target, it ceases to be a good measure");
    assert(buffer->data[10] == 's');

    release_buffer(buffer);

    printf("-> pass basic\n");
    fflush(stdout);

    // alloc a huge buffer
    struct Buffer *huge_buffer = new_buffer(60000);
    assert(huge_buffer->data != NULL);

    memset(huge_buffer->data, 0xEE, 60000);
    assert((uint8_t)(huge_buffer->data[50000]) == 0xEE);

    release_buffer(huge_buffer);

    printf("-> pass huge allocations\n");
    fflush(stdout);

    int ret = __builtin_wasm_memory_grow(0, 1);
    // sync with the value assigned by `--initial-memory`
    assert(ret == 3);

    printf("-> pass manually memory.grow\n");
    fflush(stdout);
}

int
main()
{
    goodhart_law();
    return EXIT_SUCCESS;
}