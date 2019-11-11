/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/**
 * @file wasm_thread.h
 * @brief This file contains Beihai platform abstract layer interface for
 *        thread relative function.
 */

#ifndef _WA_THREAD_H
#define _WA_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bh_thread.h"


#define ws_thread_sys_init vm_thread_sys_init

#define ws_thread_sys_destroy vm_thread_sys_destroy

#define ws_self_thread vm_self_thread

#define ws_tls_put(ptr) vm_tls_put(0, ptr)

#define ws_tls_get() vm_tls_get(0)

static inline int
ws_mutex_init(korp_mutex *mutex, bool is_recursive)
{
    if (is_recursive)
        return vm_recursive_mutex_init(mutex);
    else
        return vm_mutex_init(mutex);
}

#define ws_mutex_destroy vm_mutex_destroy

#define ws_mutex_lock vm_mutex_lock

#define ws_mutex_unlock vm_mutex_unlock

#ifdef __cplusplus
}
#endif

#endif /* end of _WA_THREAD_H */

