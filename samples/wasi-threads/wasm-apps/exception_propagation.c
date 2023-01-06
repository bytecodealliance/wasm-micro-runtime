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
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>

#include "wasi_thread_start.h"

#define TIMEOUT_SECONDS 10
#define NUM_THREADS 3
static sem_t sem;

typedef struct {
    start_args_t base;
    bool throw_exception;
} shared_t;

void
run_long_task()
{
    // Busy waiting to be interruptible by exception
    for (int i = 0; i < TIMEOUT_SECONDS; i++)
        sleep(1);
}

void
__wasi_thread_start_C(int thread_id, int *start_arg)
{
    shared_t *data = (shared_t *)start_arg;

    if (data->throw_exception) {
        // Wait for all other threads (including main thread) to be ready
        printf("Waiting before throwing exception\n");
        for (int i = 0; i < NUM_THREADS; i++)
            sem_wait(&sem);

        printf("Throwing exception\n");
        __builtin_trap();
    }
    else {
        printf("Thread running\n");

        sem_post(&sem);
        run_long_task(); // Wait to be interrupted by exception
        assert(false && "Unreachable");
    }
}

int
main(int argc, char **argv)
{
    int thread_id = -1, i;
    shared_t data[NUM_THREADS] = { 0 };

    if (sem_init(&sem, 0, 0) != 0) {
        printf("Failed to init semaphore\n");
        return EXIT_FAILURE;
    }

    for (i = 0; i < NUM_THREADS; i++) {
        // No graceful memory free to simplify the example
        if (!start_args_init(&data[i].base)) {
            printf("Failed to allocate thread's stack\n");
            return EXIT_FAILURE;
        }
    }

    // Create a thread that throws an exception
    data[0].throw_exception = true;
    thread_id = __wasi_thread_spawn(&data[0]);
    if (thread_id < 0) {
        printf("Failed to create thread: %d\n", thread_id);
        return EXIT_FAILURE;
    }
    // Create two additional threads to test exception propagation
    thread_id = __wasi_thread_spawn(&data[1]);
    if (thread_id < 0) {
        printf("Failed to create thread: %d\n", thread_id);
        return EXIT_FAILURE;
    }
    thread_id = __wasi_thread_spawn(&data[2]);
    if (thread_id < 0) {
        printf("Failed to create thread: %d\n", thread_id);
        return EXIT_FAILURE;
    }

    printf("Main thread running\n");

    sem_post(&sem);
    run_long_task(); // Wait to be interrupted by exception
    assert(false && "Unreachable");

    return EXIT_SUCCESS;
}