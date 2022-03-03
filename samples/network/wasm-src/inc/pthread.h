/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WAMR_LIB_PTHREAD_H
#define _WAMR_LIB_PTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Data type define of pthread, mutex, cond and key */
typedef unsigned int pthread_t;
typedef unsigned int pthread_mutex_t;
typedef unsigned int pthread_cond_t;
typedef unsigned int pthread_key_t;

/* Thread APIs */
int
pthread_create(pthread_t *thread, const void *attr,
               void *(*start_routine)(void *), void *arg)
    __attribute__((__import_module__("wamr_ext"),
                   __import_name__("pthread_create")));

int
pthread_join(pthread_t thread, void **retval)
    __attribute__((__import_module__("wamr_ext_a"), __import_name__("pthread_join")));

int
pthread_detach(pthread_t thread)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_detach")));

int
pthread_cancel(pthread_t thread)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_cancel")));

pthread_t
pthread_self(void)
    __attribute__((__import_module__("env"), __import_name__("pthread_self")));

void
pthread_exit(void *retval)
    __attribute__((__import_module__("env"), __import_name__("pthread_exit")));

/* Mutex APIs */
int
pthread_mutex_init(pthread_mutex_t *mutex, const void *attr)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_mutex_init")));

int
pthread_mutex_lock(pthread_mutex_t *mutex)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_mutex_lock")));

int
pthread_mutex_unlock(pthread_mutex_t *mutex)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_mutex_unlock")));

int
pthread_mutex_destroy(pthread_mutex_t *mutex)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_mutex_destroy")));

/* Cond APIs */
int
pthread_cond_init(pthread_cond_t *cond, const void *attr)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_cond_init")));

int
pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_cond_wait")));

int
pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                       uint64_t useconds)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_cond_timedwait")));

int
pthread_cond_signal(pthread_cond_t *cond)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_cond_signal")));

int
pthread_cond_destroy(pthread_cond_t *cond)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_cond_destroy")));

/* Pthread key APIs */
int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_key_create")));

int
pthread_setspecific(pthread_key_t key, const void *value)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_setspecific")));

void *
pthread_getspecific(pthread_key_t key)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_getspecific")));

int
pthread_key_delete(pthread_key_t key)
    __attribute__((__import_module__("env"),
                   __import_name__("pthread_key_delete")));

#ifdef __cplusplus
}
#endif

#endif /* end of _WAMR_LIB_PTHREAD_H */
