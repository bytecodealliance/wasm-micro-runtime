/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_thread.h"
#include "bh_assert.h"
#include "bh_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <sgx_thread.h>

int _vm_thread_sys_init()
{
    return 0;
}

void vm_thread_sys_destroy(void)
{
}

int _vm_thread_create_with_prio(korp_tid *tid, thread_start_routine_t start,
                                void *arg, unsigned int stack_size, int prio)
{
    return BHT_ERROR;
    // return BHT_OK;
}

int _vm_thread_create(korp_tid *tid, thread_start_routine_t start, void *arg,
                      unsigned int stack_size)
{
    return _vm_thread_create_with_prio(tid, start, arg, stack_size,
                                       BH_THREAD_DEFAULT_PRIORITY);
}

korp_tid _vm_self_thread()
{
    return sgx_thread_self();
}

void vm_thread_exit(void * code)
{
}

// storage for one thread
static __thread void *_tls_store = NULL;

void *_vm_tls_get(unsigned idx)
{
    return _tls_store;
}

int _vm_tls_put(unsigned idx, void * tls)
{
    _tls_store = tls;
    return BHT_OK;
    //return BHT_ERROR;
}

int _vm_mutex_init(korp_mutex *mutex)
{
    sgx_thread_mutex_t m = SGX_THREAD_MUTEX_INITIALIZER;
    *mutex = m;
    return BHT_OK;
}

int _vm_recursive_mutex_init(korp_mutex *mutex)
{
    sgx_thread_mutex_t m = SGX_THREAD_RECURSIVE_MUTEX_INITIALIZER;
    *mutex = m;
    return BHT_OK;
}

int _vm_mutex_destroy(korp_mutex *mutex)
{
    sgx_thread_mutex_destroy(mutex);
    return BHT_OK;
}

/* Returned error (EINVAL, EAGAIN and EDEADLK) from
 locking the mutex indicates some logic error present in
 the program somewhere.
 Don't try to recover error for an existing unknown error.*/
void vm_mutex_lock(korp_mutex *mutex)
{
    sgx_thread_mutex_lock(mutex);
}

int vm_mutex_trylock(korp_mutex *mutex)
{
    return (sgx_thread_mutex_trylock(mutex) == 0? BHT_OK: BHT_ERROR);
}

/* Returned error (EINVAL, EAGAIN and EPERM) from
 unlocking the mutex indicates some logic error present
 in the program somewhere.
 Don't try to recover error for an existing unknown error.*/
void vm_mutex_unlock(korp_mutex *mutex)
{
    sgx_thread_mutex_unlock(mutex);
}

int _vm_sem_init(korp_sem* sem, unsigned int c)
{
    return BHT_OK;
    //return BHT_ERROR;
}

int _vm_sem_destroy(korp_sem *sem)
{
    return BHT_OK;
    //return BHT_ERROR;
}

int _vm_sem_wait(korp_sem *sem)
{
    return BHT_OK;
    //return BHT_ERROR;
}

int _vm_sem_reltimedwait(korp_sem *sem, int mills)
{
    return BHT_OK;
    //return BHT_ERROR;
}

int _vm_sem_post(korp_sem *sem)
{
    return BHT_OK;
    //return BHT_ERROR;
}

int _vm_cond_init(korp_cond *cond)
{
    return BHT_OK;
    //return BHT_ERROR;
}

int _vm_cond_destroy(korp_cond *cond)
{
    return BHT_OK;
    //return BHT_ERROR;
}

int _vm_cond_wait(korp_cond *cond, korp_mutex *mutex)
{
    return BHT_OK;
    //return BHT_ERROR;
}

int _vm_cond_reltimedwait(korp_cond *cond, korp_mutex *mutex, int mills)
{
    return BHT_OK;
    //return BHT_ERROR;
}

int _vm_cond_signal(korp_cond *cond)
{
    return BHT_OK;
    //return BHT_ERROR;
}

int _vm_cond_broadcast(korp_cond *cond)
{
    return BHT_OK;
    //return BHT_ERROR;
}

int _vm_thread_cancel(korp_tid thread)
{
    return 0;
}

int _vm_thread_join(korp_tid thread, void **value_ptr, int mills)
{
    return 0;
}

int _vm_thread_detach(korp_tid thread)
{
    return 0;
}
