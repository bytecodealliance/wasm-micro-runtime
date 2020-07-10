/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef PLATFORM_API_EXTENSION_H
#define PLATFORM_API_EXTENSION_H

#include "platform_common.h"
/**
 * The related data structures should be defined
 * in platform_internal.h
 **/
#include "platform_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************
 *                                                 *
 *                Extension interface              *
 *                                                 *
 ***************************************************/

/**
 * NOTES:
 * 1. If you are building VM core only, it must be implemented to
 *    enable multi-thread support, otherwise no need to implement it
 * 2. To build the app-mgr and app-framework, you must implement it
 */


/**
 * Ceates a thread
 *
 * @param p_tid  [OUTPUT] the pointer of tid
 * @param start  main routine of the thread
 * @param arg  argument passed to main routine
 * @param stack_size  bytes of stack size
 *
 * @return 0 if success.
 */
int os_thread_create(korp_tid *p_tid, thread_start_routine_t start, void *arg,
                     unsigned int stack_size);

/**
 * Creates a thread with priority
 *
 * @param p_tid  [OUTPUT] the pointer of tid
 * @param start  main routine of the thread
 * @param arg  argument passed to main routine
 * @param stack_size  bytes of stack size
 * @param prio the priority
 *
 * @return 0 if success.
 */
int os_thread_create_with_prio(korp_tid *p_tid, thread_start_routine_t start,
                               void *arg, unsigned int stack_size, int prio);

/**
 * Waits for the thread specified by thread to terminate
 *
 * @param thread the thread to wait
 * @param retval if not NULL, output the exit status of the terminated thread
 *
 * @return return 0 if success
 */
int os_thread_join(korp_tid thread, void **retval);

/**
 * Detach the thread specified by thread
 *
 * @param thread the thread to detach
 *
 * @return return 0 if success
 */
int os_thread_detach(korp_tid);

/**
 * Exit current thread
 *
 * @param retval the return value of the current thread
 */
void os_thread_exit(void *retval);

/**
 * Suspend execution of the calling thread for (at least)
 * usec microseconds
 *
 * @param return 0 if success, -1 otherwise
 */
int os_usleep(uint32 usec);

/**
 * Creates a recursive mutex
 *
 * @param mutex [OUTPUT] pointer to mutex initialized.
 *
 * @return 0 if success
 */
int os_recursive_mutex_init(korp_mutex *mutex);

/**
 * This function creates a condition variable
 *
 * @param cond [OUTPUT] pointer to condition variable
 *
 * @return 0 if success
 */
int os_cond_init(korp_cond *cond);

/**
 * This function destroys condition variable
 *
 * @param cond pointer to condition variable
 *
 * @return 0 if success
 */
int os_cond_destroy(korp_cond *cond);

/**
 * Wait a condition variable.
 *
 * @param cond pointer to condition variable
 * @param mutex pointer to mutex to protect the condition variable
 *
 * @return 0 if success
 */
int os_cond_wait(korp_cond *cond, korp_mutex *mutex);

/**
 * Wait a condition varible or return if time specified passes.
 *
 * @param cond pointer to condition variable
 * @param mutex pointer to mutex to protect the condition variable
 * @param useconds microseconds to wait
 *
 * @return 0 if success
 */
int os_cond_reltimedwait(korp_cond *cond, korp_mutex *mutex, int useconds);

/**
 * Signals the condition variable
 *
 * @param cond condition variable
 *
 * @return 0 if success
 */
int os_cond_signal(korp_cond *cond);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef PLATFORM_API_EXTENSION_H */
