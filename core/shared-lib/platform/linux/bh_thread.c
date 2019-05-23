/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bh_thread.h"
#include "bh_assert.h"
#include "bh_log.h"
#include "bh_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static bool is_thread_sys_inited = false;

static korp_mutex thread_list_lock;
static pthread_key_t thread_local_storage_key[BH_MAX_TLS_NUM];

int _vm_thread_sys_init()
{
    unsigned i, j;
    int ret;

    if (is_thread_sys_inited)
        return 0;

    for (i = 0; i < BH_MAX_TLS_NUM; i++) {
        ret = pthread_key_create(&thread_local_storage_key[i], NULL);
        if (ret)
            goto fail;
    }

    ret = vm_mutex_init(&thread_list_lock);
    if (ret)
        goto fail;

    is_thread_sys_inited = true;
    return 0;

    fail: for (j = 0; j < i; j++)
        pthread_key_delete(thread_local_storage_key[j]);
    return -1;
}

void vm_thread_sys_destroy(void)
{
    if (is_thread_sys_inited) {
        unsigned i;
        for (i = 0; i < BH_MAX_TLS_NUM; i++)
            pthread_key_delete(thread_local_storage_key[i]);
        vm_mutex_destroy(&thread_list_lock);
        is_thread_sys_inited = false;
    }
}

typedef struct {
    thread_start_routine_t start;
    void* stack;
    int stack_size;
    void* arg;
} thread_wrapper_arg;

static void *vm_thread_wrapper(void *arg)
{
    thread_wrapper_arg * targ = arg;
    LOG_VERBOSE("THREAD CREATE 0x%08x\n", &targ);
    targ->stack = (void *) ((unsigned int) (&arg) & ~0xfff);
    _vm_tls_put(1, targ);
    targ->start(targ->arg);
    bh_free(targ);
    _vm_tls_put(1, NULL);
    return NULL;
}

int _vm_thread_create_with_prio(korp_tid *tid, thread_start_routine_t start,
        void *arg, unsigned int stack_size, int prio)
{
    pthread_attr_t tattr;
    thread_wrapper_arg *targ;

    bh_assert(stack_size > 0);
    bh_assert(tid);
    bh_assert(start);

    *tid = INVALID_THREAD_ID;

    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_JOINABLE);
    if (pthread_attr_setstacksize(&tattr, stack_size) != 0) {
        bh_debug("Invalid thread stack size %u. Min stack size on Linux = %u",
                stack_size, PTHREAD_STACK_MIN);
        pthread_attr_destroy(&tattr);
        return BHT_ERROR;
    }

    targ = (thread_wrapper_arg*) bh_malloc(sizeof(*targ));
    if (!targ) {
        pthread_attr_destroy(&tattr);
        return BHT_ERROR;
    }

    targ->start = start;
    targ->arg = arg;
    targ->stack_size = stack_size;

    if (pthread_create(tid, &tattr, vm_thread_wrapper, targ) != 0) {
        pthread_attr_destroy(&tattr);
        bh_free(targ);
        return BHT_ERROR;
    }

    pthread_attr_destroy(&tattr);
    return BHT_OK;
}

int _vm_thread_create(korp_tid *tid, thread_start_routine_t start, void *arg,
        unsigned int stack_size)
{
    return _vm_thread_create_with_prio(tid, start, arg, stack_size,
                                       BH_THREAD_DEFAULT_PRIORITY);
}

korp_tid _vm_self_thread()
{
    return (korp_tid) pthread_self();
}

void vm_thread_exit(void * code)
{
    bh_free(_vm_tls_get(1));
    _vm_tls_put(1, NULL);
    pthread_exit(code);
}

void *_vm_tls_get(unsigned idx)
{
    bh_assert(idx < BH_MAX_TLS_NUM);
    return pthread_getspecific(thread_local_storage_key[idx]);
}

int _vm_tls_put(unsigned idx, void * tls)
{
    bh_assert(idx < BH_MAX_TLS_NUM);
    pthread_setspecific(thread_local_storage_key[idx], tls);
    return BHT_OK;
}

int _vm_mutex_init(korp_mutex *mutex)
{
    return pthread_mutex_init(mutex, NULL) == 0 ? BHT_OK : BHT_ERROR;
}

int _vm_recursive_mutex_init(korp_mutex *mutex)
{
    int ret;

    pthread_mutexattr_t mattr;

    bh_assert(mutex);
    ret = pthread_mutexattr_init(&mattr);
    if (ret)
        return BHT_ERROR;

    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE_NP);
    ret = pthread_mutex_init(mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);

    return ret == 0 ? BHT_OK : BHT_ERROR;
}

int _vm_mutex_destroy(korp_mutex *mutex)
{
    int ret;

    bh_assert(mutex);
    ret = pthread_mutex_destroy(mutex);

    return ret == 0 ? BHT_OK : BHT_ERROR;
}

/* Returned error (EINVAL, EAGAIN and EDEADLK) from
 locking the mutex indicates some logic error present in
 the program somewhere.
 Don't try to recover error for an existing unknown error.*/
void vm_mutex_lock(korp_mutex *mutex)
{
    int ret;

    bh_assert(mutex);
    ret = pthread_mutex_lock(mutex);
    if (0 != ret) {
        printf("vm mutex lock failed (ret=%d)!\n", ret);
        exit(-1);
    }
}

int vm_mutex_trylock(korp_mutex *mutex)
{
    int ret;

    bh_assert(mutex);
    ret = pthread_mutex_trylock(mutex);

    return ret == 0 ? BHT_OK : BHT_ERROR;
}

/* Returned error (EINVAL, EAGAIN and EPERM) from
 unlocking the mutex indicates some logic error present
 in the program somewhere.
 Don't try to recover error for an existing unknown error.*/
void vm_mutex_unlock(korp_mutex *mutex)
{
    int ret;

    bh_assert(mutex);
    ret = pthread_mutex_unlock(mutex);
    if (0 != ret) {
        printf("vm mutex unlock failed (ret=%d)!\n", ret);
        exit(-1);
    }
}

int _vm_sem_init(korp_sem* sem, unsigned int c)
{
    int ret;

    bh_assert(sem);
    ret = sem_init(sem, 0, c);

    return ret == 0 ? BHT_OK : BHT_ERROR;
}

int _vm_sem_destroy(korp_sem *sem)
{
    int ret;

    bh_assert(sem);
    ret = sem_destroy(sem);

    return ret == 0 ? BHT_OK : BHT_ERROR;
}

int _vm_sem_wait(korp_sem *sem)
{
    int ret;

    bh_assert(sem);

    ret = sem_wait(sem);

    return ret == 0 ? BHT_OK : BHT_ERROR;
}

int _vm_sem_reltimedwait(korp_sem *sem, int mills)
{
    int ret = BHT_OK;

    struct timespec timeout;
    const int mills_per_sec = 1000;
    const int mills_to_nsec = 1E6;

    bh_assert(sem);

    if (mills == BHT_WAIT_FOREVER) {
        ret = sem_wait(sem);
    } else {

        timeout.tv_sec = mills / mills_per_sec;
        timeout.tv_nsec = (mills % mills_per_sec) * mills_to_nsec;
        timeout.tv_sec += time(NULL);

        ret = sem_timedwait(sem, &timeout);
    }

    if (ret != BHT_OK) {
        if (errno == BHT_TIMEDOUT) {
            ret = BHT_TIMEDOUT;
            errno = 0;
        } else {
            bh_debug("Faliure happens when timed wait is called");
            bh_assert(0);
        }
    }

    return ret;
}

int _vm_sem_post(korp_sem *sem)
{
    bh_assert(sem);

    return sem_post(sem) == 0 ? BHT_OK : BHT_ERROR;
}

int _vm_cond_init(korp_cond *cond)
{
    bh_assert(cond);

    if (pthread_cond_init(cond, NULL) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

int _vm_cond_destroy(korp_cond *cond)
{
    bh_assert(cond);

    if (pthread_cond_destroy(cond) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

int _vm_cond_wait(korp_cond *cond, korp_mutex *mutex)
{
    bh_assert(cond);
    bh_assert(mutex);

    if (pthread_cond_wait(cond, mutex) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

static void msec_nsec_to_abstime(struct timespec *ts, int64 msec, int32 nsec)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    ts->tv_sec = tv.tv_sec + msec / 1000;
    ts->tv_nsec = tv.tv_usec * 1000 + (msec % 1000) * 1000000 + nsec;

    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

int _vm_cond_reltimedwait(korp_cond *cond, korp_mutex *mutex, int mills)
{
    int ret;
    struct timespec abstime;

    if (mills == BHT_WAIT_FOREVER)
        ret = pthread_cond_wait(cond, mutex);
    else {
        msec_nsec_to_abstime(&abstime, mills, 0);
        ret = pthread_cond_timedwait(cond, mutex, &abstime);
    }

    if (ret != BHT_OK && ret != BHT_TIMEDOUT)
        return BHT_ERROR;

    return BHT_OK;
}

int _vm_cond_signal(korp_cond *cond)
{
    bh_assert(cond);

    if (pthread_cond_signal(cond) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

int _vm_cond_broadcast(korp_cond *cond)
{
    bh_assert(cond);

    if (pthread_cond_broadcast(cond) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

int _vm_thread_cancel(korp_tid thread)
{
    return pthread_cancel(thread);
}

int _vm_thread_join(korp_tid thread, void **value_ptr, int mills)
{
    return pthread_join(thread, value_ptr);
}

int _vm_thread_detach(korp_tid thread)
{
    return pthread_detach(thread);
}

