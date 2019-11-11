/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_thread.h"
#include "bh_assert.h"
#include "bh_log.h"
#include "bh_memory.h"

#include <windows.h>
#include <process.h>

#ifdef _DEBUG
#define THREAD_STACK_ADJUSTMENT (32 * 1024)
#else
#define THREAD_STACK_ADJUSTMENT 0
#endif

static korp_mutex thread_list_lock;
static DWORD tls_indexes[BH_MAX_TLS_NUM];

typedef struct {
    int zero_padding;
    thread_start_routine_t start;
    void* stack;
    void* args;
    int stack_size;
} vm_thread_block;

static DWORD tb_index;

int _vm_thread_sys_init()
{
    unsigned int i;
    for (i = 0; i < BH_MAX_TLS_NUM; i++) {
        tls_indexes[i] = TlsAlloc();
        if (tls_indexes[i] == TLS_OUT_OF_INDEXES)
            return BHT_ERROR;
    }

    tb_index = TlsAlloc();
    if (tb_index == TLS_OUT_OF_INDEXES)
        return BHT_ERROR;

    return vm_mutex_init(&thread_list_lock);
}

static unsigned int BH_ROUTINE_MODIFIER beihai_starter(void* arg)
{
    vm_thread_block* tb = (vm_thread_block*) arg;
    TlsSetValue(tb_index, tb);
    tb->stack = (void *) &arg;
    tb->start(tb->args);

    return 0;
}

int _vm_thread_create(korp_tid *tid, thread_start_routine_t start, void *arg,
        unsigned int stack_size)
{
    unsigned int default_stack_size = 20 * 1024;
    vm_thread_block* tb;
    bh_assert(tid);
    bh_assert(start);

    if (stack_size == 0)
        stack_size = default_stack_size;

#ifdef _DEBUG
    stack_size = THREAD_STACK_ADJUSTMENT + stack_size*3;
#endif

    tb = (vm_thread_block*) bh_malloc(sizeof(*tb));
    if (tb == NULL)
        return BHT_ERROR;

    memset(tb, 0, sizeof(*tb));

    tb->start = start;
    tb->stack_size = stack_size;
    tb->args = arg;

    *tid = (korp_tid) _beginthreadex(NULL, stack_size, beihai_starter,
            (void*) tb, 0, NULL);

    /* TODO: to deal with the handle; how to close it? */
    return (*tid == INVALID_THREAD_ID) ? BHT_ERROR : BHT_OK;
}

korp_tid _vm_self_thread()
{
    return (korp_tid) GetCurrentThread();
}

void vm_thread_exit(void *code)
{
    vm_thread_block *tb = (vm_thread_block*) TlsGetValue(tb_index);
    bh_free(tb);
    _endthreadex((unsigned int) code);
}

void* vm_get_stackaddr()
{
    vm_thread_block *tb = (vm_thread_block*) TlsGetValue(tb_index);
    return (char *) tb->stack + THREAD_STACK_ADJUSTMENT - tb->stack_size;
}

void *_vm_tls_get(unsigned idx)
{
    bh_assert(idx < BH_MAX_TLS_NUM);
    return TlsGetValue(tls_indexes[idx]);
}

int _vm_tls_put(unsigned idx, void *tls)
{
    BOOL r;

    bh_assert(idx < BH_MAX_TLS_NUM);
    r = TlsSetValue(tls_indexes[idx], tls);
    return (r == FALSE) ? BHT_ERROR : BHT_OK;
}

int _vm_mutex_init(korp_mutex *mutex)
{
    bh_assert(mutex);
    *mutex = CreateMutex(NULL, FALSE, NULL);
    return (*mutex == 0) ? BHT_ERROR : BHT_OK;
}

int _vm_mutex_destroy(korp_mutex *mutex)
{
    return BHT_OK;
}

/* Returned error (e.g. ERROR_INVALID_HANDLE) from
 locking the mutex indicates some logic error present in
 the program somewhere.
 Don't try to recover error for an existing unknown error.*/
void vm_mutex_lock(korp_mutex *mutex)
{
    DWORD ret;

    bh_assert(mutex);
    ret = WaitForSingleObject(*mutex, INFINITE);
    if (WAIT_FAILED == ret) {
        LOG_FATAL("vm mutex lock failed (ret=%d)!\n", GetLastError());
        exit(-1);
    }
}

int vm_mutex_trylock(korp_mutex *mutex)
{
    DWORD ret;

    bh_assert(mutex);
    ret = WaitForSingleObject(*mutex, 0);
    if (WAIT_FAILED == ret) {
        LOG_FATAL("vm mutex lock failed (ret=%d)!\n", GetLastError());
        exit(-1);
    }
    return ret == WAIT_OBJECT_0 ? BHT_OK : BHT_ERROR;
}

/* Returned error (e.g. ERROR_INVALID_HANDLE) from
 unlocking the mutex indicates some logic error present
 in the program somewhere.
 Don't try to recover error for an existing unknown error.*/
void vm_mutex_unlock(korp_mutex *mutex)
{
    BOOL ret;

    bh_assert(mutex);
    ret = ReleaseMutex(*mutex);
    if (FALSE == ret) {
        LOG_FATAL("vm mutex unlock failed (ret=%d)!\n", GetLastError());
        exit(-1);
    }
}

#define BH_SEM_COUNT_MAX 0xFFFF

int _vm_sem_init(korp_sem *sem, unsigned int count)
{
    bh_assert(sem);
    bh_assert(count <= BH_SEM_COUNT_MAX);
    *sem = CreateSemaphore(NULL, count, BH_SEM_COUNT_MAX, NULL);

    return (*sem == NULL) ? BHT_ERROR : BHT_OK;
}

int _vm_sem_destroy(korp_sem *sem)
{
    return BHT_OK;
}

int _vm_sem_P(korp_sem *sem)
{
    DWORD r;
    bh_assert(sem);
    r = WaitForSingleObject(*sem, INFINITE);

    return (r == WAIT_FAILED) ? BHT_ERROR : BHT_OK;
}

int _vm_sem_reltimedP(korp_sem *sem, int mills)
{
    DWORD r;
    bh_assert(sem);

    if (mills == BHT_WAIT_FOREVER)
        mills = INFINITE;

    r = WaitForSingleObject(*sem, (unsigned int) mills);

    switch (r) {
    case WAIT_OBJECT_0:
        return BHT_OK;
    case WAIT_TIMEOUT:
        return BHT_TIMEDOUT;
    default:
        return BHT_ERROR;
    }
}

int _vm_sem_V(korp_sem *sem)
{
    BOOL r;
    bh_assert(sem);
    r = ReleaseSemaphore(*sem, 1, NULL);
    return (r == FALSE) ? BHT_ERROR : BHT_OK;
}

int _vm_cond_init(korp_cond *cond)
{
    bh_assert(cond);
    cond->waiting_count = 0;
    return vm_sem_init(&cond->s, 0);
}

int _vm_cond_destroy(korp_cond *cond)
{
    bh_assert(cond);
    return vm_sem_destroy(&cond->s);
}

int _vm_cond_wait(korp_cond *cond, korp_mutex *mutex)
{
    bh_assert(cond);
    bh_assert(mutex);

    cond->waiting_count++;

    vm_mutex_unlock(mutex);

    if (vm_sem_P(&cond->s) != BHT_OK)
        return BHT_ERROR;

    vm_mutex_lock(mutex);

    cond->waiting_count--;

    return BHT_OK;
}

int _vm_cond_reltimedwait(korp_cond *cond, korp_mutex *mutex, int mills)
{
    int r;

    bh_assert(cond);
    bh_assert(mutex);

    cond->waiting_count++;

    vm_mutex_unlock(mutex);

    r = vm_sem_reltimedP(&cond->s, mills);

    if ((r != BHT_OK) && (r != BHT_TIMEDOUT))
        return BHT_ERROR;

    vm_mutex_lock(mutex);

    cond->waiting_count--;

    return r;
}

int _vm_cond_signal(korp_cond *cond)
{
    bh_assert(cond);

    if (cond->waiting_count == 0)
        return BHT_OK;

    if (vm_sem_V(&cond->s) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

int _vm_cond_broadcast(korp_cond *cond)
{
    /* FIXME: use pthread's API to implement this and above
     functions.  */

    unsigned count = cond->waiting_count;

    for (; count > 0; count--)
        vm_sem_V(&cond->s);

    return BHT_OK;
}

int _vm_thread_cancel(korp_tid thread)
{
    /* FIXME: implement this with Windows API.  */
    return 0;
}

int _vm_thread_join(korp_tid thread, void **value_ptr)
{
    /* FIXME: implement this with Windows API.  */
    return 0;
}

int _vm_thread_detach(korp_tid thread)
{
    /* FIXME: implement this with Windows API.  */
    return 0;
}
