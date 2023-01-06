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

#define TEST_TERMINATION_BY_TRAP 0 // Otherwise test `proc_exit` termination
#define TEST_TERMINATION_IN_MAIN_THREAD 1

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
    // Busy waiting to be interruptible by trap or `proc_exit`
    for (int i = 0; i < TIMEOUT_SECONDS; i++)
        sleep(1);
}

void
__wasi_thread_start_C(int thread_id, int *start_arg)
{
    shared_t *data = (shared_t *)start_arg;

    if (data->throw_exception) {
        // Wait for all other threads (including main thread) to be ready
        printf("Waiting before terminating\n");
        for (int i = 0; i < NUM_THREADS; i++)
            sem_wait(&sem);

        printf("Force termination\n");
#if TEST_TERMINATION_BY_TRAP == 1
        __builtin_trap();
#else
        __wasi_proc_exit(1);
#endif
    }
    else {
        printf("Thread running\n");

        sem_post(&sem);
        run_long_task(); // Wait to be interrupted
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

// Create a thread that forces termination through trap or `proc_exit`
#if TEST_TERMINATION_IN_MAIN_THREAD == 1
    data[0].throw_exception = false;
#else
    data[0].throw_exception = true;
#endif
    thread_id = __wasi_thread_spawn(&data[0]);
    if (thread_id < 0) {
        printf("Failed to create thread: %d\n", thread_id);
        return EXIT_FAILURE;
    }

    // Create two additional threads to test exception propagation
    data[1].throw_exception = false;
    thread_id = __wasi_thread_spawn(&data[1]);
    if (thread_id < 0) {
        printf("Failed to create thread: %d\n", thread_id);
        return EXIT_FAILURE;
    }
    data[2].throw_exception = false;
    thread_id = __wasi_thread_spawn(&data[2]);
    if (thread_id < 0) {
        printf("Failed to create thread: %d\n", thread_id);
        return EXIT_FAILURE;
    }

    printf("Main thread running\n");

    sem_post(&sem);

#if TEST_TERMINATION_IN_MAIN_THREAD == 1

    printf("Force termination (main thread)\n");
#if TEST_TERMINATION_BY_TRAP == 1
    __builtin_trap();
#else  /* TEST_TERMINATION_BY_TRAP */
    __wasi_proc_exit(1);
#endif /* TEST_TERMINATION_BY_TRAP */

#else  /* TEST_TERMINATION_IN_MAIN_THREAD */
    run_long_task(); // Wait to be interrupted
    assert(false && "Unreachable");
#endif /* TEST_TERMINATION_IN_MAIN_THREAD */
    return EXIT_SUCCESS;
}