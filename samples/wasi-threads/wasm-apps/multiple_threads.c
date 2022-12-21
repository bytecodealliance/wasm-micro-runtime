/*
 * Copyright (C) 2022 Amazon.com Inc. or its affiliates. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#ifndef __wasi__
#error This example only compiles to WASM/WASI target
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <wasi/api.h>
#include <stdbool.h>

#define MAX_NUM_THREADS 4 // Default maximum number of threads for iwasm
static const int64_t SECOND = 1000 * 1000 * 1000;

typedef struct {
    int th_ready;
    int th_done;
} shared_t;

__attribute__((export_name("wasi_thread_start"))) void
wasi_thread_start(int thread_id, int *start_arg)
{
    shared_t *data = (shared_t *)start_arg;

    if (__builtin_wasm_memory_atomic_wait32(&data->th_ready, 0, SECOND) == 2)
        assert(false && "Wait operation in thread failed");

    data->th_done = 1;
    __builtin_wasm_memory_atomic_notify(&data->th_done, 1);
}

int
main(int argc, char **argv)
{
    int thread_ids[MAX_NUM_THREADS];
    shared_t data[MAX_NUM_THREADS] = { 0 };

    for (int i = 0; i < MAX_NUM_THREADS; i++) {
        thread_ids[i] = __wasi_thread_spawn(&data[i]);
        printf("Created thread with id = %d\n", thread_ids[i]);
        assert(thread_ids[i] >= 0 && "Thread ids must be >= 0");

        bool is_id_already_existing = false;
        for (int j = 0; j < i; j++) {
            if (thread_ids[i] == thread_ids[j])
                is_id_already_existing = true;
        }
        assert(!is_id_already_existing && "Thread ids must be unique");
    }

    printf("Try to create additional thread after limit reached\n");
    shared_t data_fail = { 0 };
    int thread_id = __wasi_thread_spawn(&data_fail);
    assert(thread_id < 0 && "Thread creation must fail");

    // Unlock first-created thread
    data[0].th_ready = 1;
    __builtin_wasm_memory_atomic_notify(&data[0].th_ready, 1);
    // And wait for it to return
    if (__builtin_wasm_memory_atomic_wait32(&data[0].th_done, 0, SECOND) == 2)
        assert(false && "Wait operation in main thread failed");

    printf("Try to create additional thread after previous was released\n");
    shared_t data_succeed = { 0 };
    thread_id = __wasi_thread_spawn(&data_succeed);
    assert(thread_id >= 0 && "Thread creation must succeed");

    printf("Created thread with id = %d\n", thread_id);
    assert(thread_id == thread_ids[0]
           && "New thread must reuse previously-released identifier");

    // Unlock all running threads
    data_succeed.th_ready = 1;
    __builtin_wasm_memory_atomic_notify(&data_succeed.th_ready, 1);
    for (int i = 1; i < MAX_NUM_THREADS; i++) {
        data[i].th_ready = 1;
        __builtin_wasm_memory_atomic_notify(&data[i].th_ready, 1);
    }

    return EXIT_SUCCESS;
}