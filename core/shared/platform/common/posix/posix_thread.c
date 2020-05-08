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
    printf("THREAD CREATE %p\n", &targ);
    targ->stack = (void *)((uintptr_t)(&arg) & (uintptr_t)~0xfff);
    targ->start(targ->arg);
    BH_FREE(targ);
    return NULL;
}

int os_thread_create_with_prio(korp_tid *tid, thread_start_routine_t start,
                               void *arg, unsigned int stack_size, int prio)
{
    pthread_attr_t tattr;
    thread_wrapper_arg *targ;

    assert(stack_size > 0);
    assert(tid);
    assert(start);

    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_JOINABLE);
    if (pthread_attr_setstacksize(&tattr, stack_size) != 0) {
        printf("Invalid thread stack size %u. Min stack size on Linux = %u",
               stack_size, PTHREAD_STACK_MIN);
        pthread_attr_destroy(&tattr);
        return BHT_ERROR;
    }

    targ = (thread_wrapper_arg*) BH_MALLOC(sizeof(*targ));
    if (!targ) {
        pthread_attr_destroy(&tattr);
        return BHT_ERROR;
    }

    targ->start = start;
    targ->arg = arg;
    targ->stack_size = stack_size;

    if (pthread_create(tid, &tattr, os_thread_wrapper, targ) != 0) {
        pthread_attr_destroy(&tattr);
        BH_FREE(targ);
        return BHT_ERROR;
    }

    pthread_attr_destroy(&tattr);
    return BHT_OK;
}

int os_thread_create(korp_tid *tid, thread_start_routine_t start, void *arg,
                     unsigned int stack_size)
{
    return os_thread_create_with_prio(tid, start, arg, stack_size,
                                      BH_THREAD_DEFAULT_PRIORITY);
}

korp_tid os_self_thread()
{
    return (korp_tid) pthread_self();
}

int os_mutex_init(korp_mutex *mutex)
{
    return pthread_mutex_init(mutex, NULL) == 0 ? BHT_OK : BHT_ERROR;
}

int os_recursive_mutex_init(korp_mutex *mutex)
{
    int ret;

    pthread_mutexattr_t mattr;

    assert(mutex);
    ret = pthread_mutexattr_init(&mattr);
    if (ret)
        return BHT_ERROR;

    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
    ret = pthread_mutex_init(mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);

    return ret == 0 ? BHT_OK : BHT_ERROR;
}

int os_mutex_destroy(korp_mutex *mutex)
{
    int ret;

    assert(mutex);
    ret = pthread_mutex_destroy(mutex);

    return ret == 0 ? BHT_OK : BHT_ERROR;
}

/* Returned error (EINVAL, EAGAIN and EDEADLK) from
 locking the mutex indicates some logic error present in
 the program somewhere.
 Don't try to recover error for an existing unknown error.*/
void os_mutex_lock(korp_mutex *mutex)
{
    int ret;

    assert(mutex);
    ret = pthread_mutex_lock(mutex);
    if (0 != ret) {
        printf("vm mutex lock failed (ret=%d)!\n", ret);
        exit(-1);
    }
}

/* Returned error (EINVAL, EAGAIN and EPERM) from
 unlocking the mutex indicates some logic error present
 in the program somewhere.
 Don't try to recover error for an existing unknown error.*/
void os_mutex_unlock(korp_mutex *mutex)
{
    int ret;

    assert(mutex);
    ret = pthread_mutex_unlock(mutex);
    if (0 != ret) {
        printf("vm mutex unlock failed (ret=%d)!\n", ret);
        exit(-1);
    }
}

int os_cond_init(korp_cond *cond)
{
    assert(cond);

    if (pthread_cond_init(cond, NULL) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

int os_cond_destroy(korp_cond *cond)
{
    assert(cond);

    if (pthread_cond_destroy(cond) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

int os_cond_wait(korp_cond *cond, korp_mutex *mutex)
{
    assert(cond);
    assert(mutex);

    if (pthread_cond_wait(cond, mutex) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
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
    int ret;
    struct timespec abstime;

    if (useconds == (int)BHT_WAIT_FOREVER)
        ret = pthread_cond_wait(cond, mutex);
    else {
        msec_nsec_to_abstime(&abstime, useconds);
        ret = pthread_cond_timedwait(cond, mutex, &abstime);
    }

    if (ret != BHT_OK && ret != ETIMEDOUT)
        return BHT_ERROR;

    return BHT_OK;
}

int os_cond_signal(korp_cond *cond)
{
    assert(cond);

    if (pthread_cond_signal(cond) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

int os_thread_join(korp_tid thread, void **value_ptr)
{
    return pthread_join(thread, value_ptr);
}

uint8 *os_thread_get_stack_boundary()
{
    pthread_attr_t attr;
    void *addr = NULL;
    size_t size;

    if (pthread_getattr_np(pthread_self(), &attr) == 0) {
        pthread_attr_getstack(&attr, &addr, &size);
        pthread_attr_destroy (&attr);
    }

    if (addr)
        /* Reserved 4 KB for safety */
        return (uint8*)addr + 4 * 1024;
    else
        return NULL;
}

