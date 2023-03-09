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
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#include "wasi_thread_start.h"

#define BUSY_WAIT 0
#define ATOMIC_WAIT 1
#define POLL_ONEOFF 2

/* Change parameters here to modify the sample behavior */
#define TEST_TERMINATION_BY_TRAP 0 /* Otherwise `proc_exit` termination */
#define TEST_TERMINATION_IN_MAIN_THREAD 1 /* Otherwise in spawn thread */
#define LONG_TASK_IMPL ATOMIC_WAIT

#define TIMEOUT_SECONDS 10
#define NUM_THREADS 3
static pthread_barrier_t barrier;

typedef struct {
    start_args_t base;
    bool throw_exception;
} shared_t;

void
run_long_task()
{
#if LONG_TASK_IMPL == BUSY_WAIT
    for (int i = 0; i < TIMEOUT_SECONDS; i++)
        sleep(1);
#elif LONG_TASK_IMPL == ATOMIC_WAIT
    __builtin_wasm_memory_atomic_wait32(0, 0, -1);
#else
    sleep(TIMEOUT_SECONDS);
#endif
}

void
start_job()
{
    /* Wait for all threads (including the main thread) to be ready */
    pthread_barrier_wait(&barrier);
    run_long_task(); /* Task to be interrupted */
    assert(false && "Thread termination test failed");
}

void
terminate_process()
{
    /* Wait for all other threads (including main thread) to be ready */
    printf("Waiting before terminating\n");
    pthread_barrier_wait(&barrier);

    printf("Force termination\n");
#if TEST_TERMINATION_BY_TRAP == 1
    __builtin_trap();
#else
    __wasi_proc_exit(33);
#endif
}

void
__wasi_thread_start_C(int thread_id, int *start_arg)
{
    shared_t *data = (shared_t *)start_arg;

    if (data->throw_exception) {
        terminate_process();
    }
    else {
        printf("Thread running\n");

        start_job();
    }
}

int
main(int argc, char **argv)
{
    int thread_id = -1, i;
    shared_t data[NUM_THREADS] = { 0 };

    if (pthread_barrier_init(&barrier, NULL, NUM_THREADS + 1) != 0) {
        printf("Failed to init barrier\n");
        return EXIT_FAILURE;
    }

    for (i = 0; i < NUM_THREADS; i++) {
        /* No graceful memory free to simplify the example */
        if (!start_args_init(&data[i].base)) {
            printf("Failed to allocate thread's stack\n");
            return EXIT_FAILURE;
        }
    }

    /* Create a thread that forces termination through trap or `proc_exit` */
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

    /* Create two additional threads to test exception propagation */
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

#if TEST_TERMINATION_IN_MAIN_THREAD == 1
    printf("Force termination (main thread)\n");
    terminate_process();
#else  /* TEST_TERMINATION_IN_MAIN_THREAD */
    printf("Main thread running\n");

    start_job();
#endif /* TEST_TERMINATION_IN_MAIN_THREAD */
    return EXIT_SUCCESS;
}
