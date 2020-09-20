/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "platform_api_vmcore.h"
#include "platform_api_extension.h"

typedef struct {
    thread_start_routine_t start;
    void* stack;
    uint32 stack_size;
    void* arg;
} thread_wrapper_arg;

static void *os_thread_wrapper(void *arg)
{
    thread_wrapper_arg * targ = arg;
    thread_start_routine_t start_func = targ->start;
    void *thread_arg = targ->arg;
    os_printf("THREAD CREATED %p\n", &targ);
    targ->stack = (void *)((uintptr_t)(&arg) & (uintptr_t)~0xfff);
    BH_FREE(targ);
    start_func(thread_arg);
    return NULL;
}

int os_thread_create_with_prio(korp_tid *tid, thread_start_routine_t start,
                               void *arg, unsigned int stack_size, int prio)
{
    return BHT_ERROR;
}

int os_thread_create(korp_tid *tid, thread_start_routine_t start, void *arg,
                     unsigned int stack_size)
{
    return os_thread_create_with_prio(tid, start, arg, stack_size,
                                      BH_THREAD_DEFAULT_PRIORITY);
}

korp_tid os_self_thread()
{
    return NULL;
}

int os_mutex_init(korp_mutex *mutex)
{
    return BHT_OK;
}

int os_recursive_mutex_init(korp_mutex *mutex)
{
    return BHT_OK;
}

int os_mutex_destroy(korp_mutex *mutex)
{
    return BHT_OK;
}

int os_mutex_lock(korp_mutex *mutex)
{
    return BHT_ERROR;
}

int os_mutex_unlock(korp_mutex *mutex)
{
    return BHT_OK;
}

int os_cond_init(korp_cond *cond)
{
    return BHT_OK;
}

int os_cond_destroy(korp_cond *cond)
{
    return BHT_OK;
}

int os_cond_wait(korp_cond *cond, korp_mutex *mutex)
{
    return BHT_OK;
}


int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    /* Note: some broken versions only have 8 trailing zero's,
        the correct epoch has 9 trailing zero's
       This magic number is the number of 100 nanosecond intervals
        since January 1, 1601 (UTC) until 00:00:00 January 1, 1970 */
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);

    return 0;
}

static void msec_nsec_to_abstime(struct timespec *ts, int usec)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    ts->tv_sec = (long int)(tv.tv_sec + usec / 1000000);
    ts->tv_nsec = (long int)(tv.tv_usec * 1000 + (usec % 1000000) * 1000);

    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

int os_cond_reltimedwait(korp_cond *cond, korp_mutex *mutex, int useconds)
{
    return BHT_OK;
}

int os_cond_signal(korp_cond *cond)
{
    return BHT_OK;
}

int os_thread_join(korp_tid thread, void **value_ptr)
{
    return BHT_OK;
}

int os_thread_detach(korp_tid thread)
{
    return BHT_OK;
}

void os_thread_exit(void *retval)
{
    return BHT_OK;
}

uint8 *os_thread_get_stack_boundary()
{
    return NULL;
}
