/*
 * Copyright (C) 2022 Amazon.com Inc. or its affiliates. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#ifndef __wasi__
#error This example only compiles to WASM/WASI target
#endif

#include <stdlib.h>
#include <stdio.h>
#include <wasi/api.h>

static const int64_t SECOND = 1000 * 1000 * 1000;

typedef struct {
    int th_ready;
    int value;
} shared_t;

__attribute__((export_name("wasi_thread_start"))) void
wasi_thread_start(int thread_id, int *start_arg)
{
    shared_t *data = (shared_t *)start_arg;

    printf("New thread ID: %d, starting parameter: %d\n", thread_id,
           data->value);

    data->value += 8;
    printf("Updated value: %d\n", data->value);

    data->th_ready = 1;
    __builtin_wasm_memory_atomic_notify(&data->th_ready, 1);
}

int
main(int argc, char **argv)
{
    shared_t data = { 0, 52 };
    __wasi_errno_t err;

    err = __wasi_thread_spawn(&data);
    if (err != __WASI_ERRNO_SUCCESS) {
        printf("Failed to create thread: %d\n", err);
        return EXIT_FAILURE;
    }

    if (__builtin_wasm_memory_atomic_wait32(&data.th_ready, 0, SECOND) == 2) {
        printf("Timeout\n");
        return EXIT_FAILURE;
    }

    printf("Thread completed, new value: %d\n", data.value);

    return EXIT_SUCCESS;
}
