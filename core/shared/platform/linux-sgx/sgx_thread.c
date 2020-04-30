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

void os_mutex_lock(korp_mutex *mutex)
{
    sgx_thread_mutex_lock(mutex);
}

void os_mutex_unlock(korp_mutex *mutex)
{
    sgx_thread_mutex_unlock(mutex);
}

int os_cond_init(korp_cond *cond)
{
    sgx_thread_cond_t c = SGX_THREAD_COND_INITIALIZER;
    *cond = c;
    return BHT_OK;
}

int os_cond_destroy(korp_cond *cond)
{
    sgx_thread_cond_destroy(cond);
    return BHT_OK;
}

uint8 *os_thread_get_stack_boundary()
{
    /* TODO: get sgx stack boundary */
    return NULL;
}

