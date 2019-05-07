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

static korp_mutex thread_list_lock;
static pthread_key_t thread_local_storage_key[BH_MAX_TLS_NUM];

int _vm_thread_sys_init()
{
    unsigned i;

    for (i = 0; i < BH_MAX_TLS_NUM; i++)
        pthread_key_create(&thread_local_storage_key[i], NULL);

    return vm_mutex_init(&thread_list_lock);
}

korp_tid _vm_self_thread()
{
    return (korp_tid) pthread_self();
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
        LOG_FATAL("vm mutex lock failed (ret=%d)!\n", ret);
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
        LOG_FATAL("vm mutex unlock failed (ret=%d)!\n", ret);
        exit(-1);
    }
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

