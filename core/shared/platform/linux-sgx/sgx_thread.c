/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"
#include "platform_api_extension.h"

korp_tid os_self_thread()
{
    return sgx_thread_self();
}

int os_mutex_init(korp_mutex *mutex)
{
    sgx_thread_mutex_t m = SGX_THREAD_MUTEX_INITIALIZER;
    *mutex = m;
    return BHT_OK;
}

int os_mutex_destroy(korp_mutex *mutex)
{
    sgx_thread_mutex_destroy(mutex);
    return BHT_OK;
}

/* Returned error (EINVAL, EAGAIN and EDEADLK) from
 locking the mutex indicates some logic error present in
 the program somewhere.
 Don't try to recover error for an existing unknown error.*/
void os_mutex_lock(korp_mutex *mutex)
{
    sgx_thread_mutex_lock(mutex);
}

/* Returned error (EINVAL, EAGAIN and EPERM) from
 unlocking the mutex indicates some logic error present
 in the program somewhere.
 Don't try to recover error for an existing unknown error.*/
void os_mutex_unlock(korp_mutex *mutex)
{
    sgx_thread_mutex_unlock(mutex);
}

