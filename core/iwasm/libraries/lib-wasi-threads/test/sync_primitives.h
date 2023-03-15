/*
 * Copyright (C) 2023 Amazon.com Inc. or its affiliates. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdbool.h>

/* Mutex */

typedef int mutex_t;

void
mutex_init(mutex_t *mutex)
{
    *mutex = 0;
}

static bool
try_mutex_lock(mutex_t *mutex)
{
    int expected = 0;
    return __atomic_compare_exchange_n(mutex, &expected, 1, false,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

void
mutex_lock(mutex_t *mutex)
{
    while (!try_mutex_lock(mutex))
        __builtin_wasm_memory_atomic_wait32(mutex, 1, -1);
}

void
mutex_unlock(mutex_t *mutex)
{
    __atomic_store_n(mutex, 0, __ATOMIC_SEQ_CST);
    __builtin_wasm_memory_atomic_notify(mutex, 1);
}

/* Barrier */

typedef struct {
    int count;
    int num_threads;
    int mutex;
    int ready;
} barrier_t;

void
barrier_init(barrier_t *barrier, int num_threads)
{
    barrier->count = 0;
    barrier->num_threads = num_threads;
    barrier->mutex = 0;
    barrier->ready = 0;
}

void
barrier_wait(barrier_t *barrier)
{
    bool no_wait;

    mutex_lock(&barrier->mutex);
    barrier->count++;
    no_wait = (barrier->count >= barrier->num_threads);
    mutex_unlock(&barrier->mutex);

    if (no_wait) {
        __atomic_store_n(&barrier->ready, 1, __ATOMIC_SEQ_CST);
        __builtin_wasm_memory_atomic_notify(&barrier->ready, 1);
        return;
    }

    __builtin_wasm_memory_atomic_wait32(&barrier->ready, 0, -1);
}