/*
 * Copyright (C) 2023 Amazon.com Inc. or its affiliates. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef __wasi__
#error This example only compiles to WASM/WASI target
#endif

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum CONSTANTS {
    NUM_ITER = 100000,
    NUM_RETRY = 8,
    MAX_NUM_THREADS = 12,
    RETRY_SLEEP_TIME_US = 2000,
};

unsigned prime_numbers_count = 0;

bool
is_prime(unsigned int num)
{
    for (unsigned int i = 2; i <= (unsigned int)(sqrt(num)); ++i) {
        if (num % i == 0) {
            return false;
        }
    }

    return true;
}

void *
check_if_prime(void *value)
{
    unsigned int *num = (unsigned int *)(value);
    usleep(10000);
    if (is_prime(*num)) {
        __atomic_fetch_add(&prime_numbers_count, 1, __ATOMIC_SEQ_CST);
    }
    return NULL;
}

unsigned int
validate()
{
    unsigned int counter = 0;
    for (unsigned int i = 2; i <= NUM_ITER; ++i) {
        counter += is_prime(i);
    }

    return counter;
}

void
spawn_thread(pthread_t *thread, unsigned int *arg)
{
    int status_code = -1;
    int timeout_us = RETRY_SLEEP_TIME_US;
    for (int tries = 0; status_code != 0 && tries < NUM_RETRY; ++tries) {
        status_code = pthread_create(thread, NULL, &check_if_prime, arg);
        assert(status_code == 0 || status_code == EAGAIN);
        if (status_code == EAGAIN) {
            usleep(timeout_us);
            timeout_us *= 2;
        }
    }

    assert(status_code == 0 && "Thread creation should succeed");
}

int
main(int argc, char **argv)
{
    pthread_t threads[MAX_NUM_THREADS];
    unsigned int args[MAX_NUM_THREADS];
    double percentage = 0.1;

    for (unsigned int factorised_number = 2; factorised_number < NUM_ITER;
         ++factorised_number) {
        if (factorised_number > NUM_ITER * percentage) {
            fprintf(stderr, "Stress test is %d%% finished\n",
                    (unsigned int)(percentage * 100));
            percentage += 0.1;
        }

        unsigned int thread_num = factorised_number % MAX_NUM_THREADS;
        if (threads[thread_num] != 0) {
            assert(pthread_join(threads[thread_num], NULL) == 0);
        }

        args[thread_num] = factorised_number;

        usleep(RETRY_SLEEP_TIME_US);
        spawn_thread(&threads[thread_num], &args[thread_num]);
        assert(threads[thread_num] != 0);
    }

    for (int i = 0; i < MAX_NUM_THREADS; ++i) {
        assert(threads[i] == 0 || pthread_join(threads[i], NULL) == 0);
    }

    // Check the test results
    assert(
        prime_numbers_count == validate()
        && "Answer mismatch between tested code and reference implementation");

    fprintf(stderr, "Stress test finished successfully\n");
    return 0;
}
