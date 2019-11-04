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

#ifndef _BH_THREAD_H
#define _BH_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bh_config.h"
#include "bh_platform.h"

#define BH_MAX_THREAD 32
#define BH_MAX_TLS_NUM 2

#define BHT_ERROR (-1)
#define BHT_TIMED_OUT (1)
#define BHT_OK (0)

#define BHT_NO_WAIT 0x00000000
#define BHT_WAIT_FOREVER 0xFFFFFFFF

/**
 * vm_thread_sys_init
 *    initiation function for beihai thread system. Invoked at the beginning of beihai intiation.
 *
 * @return 0 if succuess.
 */
int _vm_thread_sys_init(void);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_thread_sys_init_instr(const char*func_name);
#define vm_thread_sys_init(void) vm_thread_sys_init_instr(__FUNCTION__)
#else
#define vm_thread_sys_init _vm_thread_sys_init
#endif

void vm_thread_sys_destroy(void);

/**
 * This function creates a thread
 *
 * @param p_tid  [OUTPUT] the pointer of tid
 * @param start  main routine of the thread
 * @param arg  argument passed to main routine
 * @param stack_size  bytes of stack size
 *
 * @return 0 if success.
 */
int _vm_thread_create(korp_tid *p_tid, thread_start_routine_t start, void *arg,
        unsigned int stack_size);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_thread_create_instr(korp_tid *p_tid, thread_start_routine_t start, void *arg, unsigned int stack_size, const char*func_name);
#define vm_thread_create(p_tid, start, arg, stack_size) vm_thread_create_instr(p_tid, start, arg, stack_size, __FUNCTION__)
#else
#define vm_thread_create _vm_thread_create
#endif

/**
 * This function creates a thread
 *
 * @param p_tid  [OUTPUT] the pointer of tid
 * @param start  main routine of the thread
 * @param arg  argument passed to main routine
 * @param stack_size  bytes of stack size
 * @param prio the priority
 *
 * @return 0 if success.
 */
int _vm_thread_create_with_prio(korp_tid *p_tid, thread_start_routine_t start,
        void *arg, unsigned int stack_size, int prio);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_thread_create_with_prio_instr(korp_tid *p_tid, thread_start_routine_t start, void *arg, unsigned int stack_size, int prio, const char*func_name);
#define vm_thread_create_with_prio(p_tid, start, arg, stack_size) vm_thread_create_instr(p_tid, start, arg, stack_size, prio, __FUNCTION__)
#else
#define vm_thread_create_with_prio _vm_thread_create_with_prio
#endif

/**
 * This function never returns.
 *
 * @param code not used
 */
void vm_thread_exit(void *code);

/**
 * This function gets current thread id
 *
 * @return current thread id
 */
korp_tid _vm_self_thread(void);
#ifdef _INSTRUMENT_TEST_ENABLED
korp_tid vm_self_thread_instr(const char*func_name);
#define vm_self_thread(void) vm_self_thread_instr(__FUNCTION__)
#else
#define vm_self_thread _vm_self_thread
#endif

/**
 * This function saves a pointer in thread local storage. One thread can only save one pointer.
 *
 * @param idx  tls array index
 * @param ptr  pointer need save as TLS
 *
 * @return 0 if success
 */
int _vm_tls_put(unsigned idx, void *ptr);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_tls_put_instr(unsigned idx, void *ptr, const char*func_name);
#define vm_tls_put(idx, ptr) vm_tls_put_instr(idx, ptr, __FUNCTION__)
#else
#define vm_tls_put _vm_tls_put
#endif

/**
 * This function gets a pointer saved in TLS.
 *
 * @param idx  tls array index
 *
 * @return the pointer saved in TLS.
 */
void *_vm_tls_get(unsigned idx);
#ifdef _INSTRUMENT_TEST_ENABLED
void *vm_tls_get_instr(unsigned idx, const char*func_name);
#define vm_tls_get(idx) vm_tls_get_instr(idx, __FUNCTION__)
#else
#define vm_tls_get _vm_tls_get
#endif

#define vm_thread_testcancel(void)

/**
 * This function creates a non-recursive mutex
 *
 * @param mutex [OUTPUT] pointer to mutex initialized.
 *
 * @return 0 if success
 */
int _vm_mutex_init(korp_mutex *mutex);
#ifdef INSTRUMENT_TEST_ENABLED
int vm_mutex_init_instr(korp_mutex *mutex, const char*func_name);
#define vm_mutex_init(mutex) vm_mutex_init_instr(mutex, __FUNCTION__)
#else
#define vm_mutex_init _vm_mutex_init
#endif

/**
 * This function creates a recursive mutex
 *
 * @param mutex [OUTPUT] pointer to mutex initialized.
 *
 * @return 0 if success
 */
int _vm_recursive_mutex_init(korp_mutex *mutex);
#ifdef INSTRUMENT_TEST_ENABLED
int vm_recursive_mutex_init_instr(korp_mutex *mutex, const char*func_name);
#define vm_recursive_mutex_init(mutex) vm_recursive_mutex_init_instr(mutex, __FUNCTION__)
#else
#define vm_recursive_mutex_init _vm_recursive_mutex_init
#endif

/**
 * This function destroys a mutex
 *
 * @param mutex  pointer to mutex need destroy
 *
 * @return 0 if success
 */
int _vm_mutex_destroy(korp_mutex *mutex);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_mutex_destroy_instr(korp_mutex *mutex, const char*func_name);
#define vm_mutex_destroy(mutex) vm_mutex_destroy_instr(mutex, __FUNCTION__)
#else
#define vm_mutex_destroy _vm_mutex_destroy
#endif

/**
 * This function locks the mutex
 *
 * @param mutex  pointer to mutex need lock
 *
 * @return Void
 */
void vm_mutex_lock(korp_mutex *mutex);

/**
 * This function locks the mutex without waiting
 *
 * @param mutex  pointer to mutex need lock
 *
 * @return 0 if success
 */
int vm_mutex_trylock(korp_mutex *mutex);

/**
 * This function unlocks the mutex
 *
 * @param mutex  pointer to mutex need unlock
 *
 * @return Void
 */
void vm_mutex_unlock(korp_mutex *mutex);

/**
 * This function creates a semaphone
 *
 * @param sem [OUTPUT] pointer to semaphone
 * @param c counter of semaphone
 *
 * @return 0 if success
 */
int _vm_sem_init(korp_sem *sem, unsigned int c);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_sem_init_instr(korp_sem *sem, unsigned int c, const char*func_name);
#define vm_sem_init(sem, c) vm_sem_init_instr(sem, c, __FUNCTION__)
#else
#define vm_sem_init _vm_sem_init
#endif

/**
 * This function destroys a semaphone
 *
 * @param sem pointer to semaphone need destroy
 *
 * @return 0 if success
 */
int _vm_sem_destroy(korp_sem *sem);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_sem_destroy_instr(korp_sem *sem, const char*func_name);
#define vm_sem_destroy(sem) vm_sem_destroy_instr(sem, __FUNCTION__)
#else
#define vm_sem_destroy _vm_sem_destroy
#endif

/**
 * This function performs wait operation on semaphone
 *
 * @param sem pointer to semaphone need perform wait operation
 *
 * @return 0 if success
 */
int _vm_sem_wait(korp_sem *sem);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_sem_wait_instr(korp_sem *sem, const char*func_name);
#define vm_sem_wait(sem) vm_sem_wait_instr(sem, __FUNCTION__)
#else
#define vm_sem_wait _vm_sem_wait
#endif

/**
 * This function performs wait operation on semaphone with a timeout
 *
 * @param sem  pointer to semaphone need perform wait operation
 * @param mills  wait milliseconds to return
 *
 * @return 0 if success
 * @return BH_TIMEOUT if time out
 */
int _vm_sem_reltimedwait(korp_sem *sem, int mills);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_sem_reltimedwait_instr(korp_sem *sem, int mills, const char*func_name);
#define vm_sem_reltimedwait(sem, mills) vm_sem_reltimedwait_instr(sem, mills, __FUNCTION__)
#else
#define vm_sem_reltimedwait _vm_sem_reltimedwait
#endif

/**
 * This function performs post operation on semaphone
 *
 * @param sem  pointer to semaphone need perform post operation
 *
 * @return 0 if success
 */
int _vm_sem_post(korp_sem *sem);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_sem_post_instr(korp_sem *sem, const char*func_name);
#define vm_sem_post(sem) vm_sem_post_instr(sem, __FUNCTION__)
#else
#define vm_sem_post _vm_sem_post
#endif

/**
 * This function creates a condition variable
 *
 * @param cond  [OUTPUT] pointer to condition variable
 *
 * @return 0 if success
 */
int _vm_cond_init(korp_cond *cond);
#ifdef INSTRUMENT_TEST_ENABLED
int vm_cond_init_instr(korp_cond *cond, const char*func_name);
#define vm_cond_init(cond) vm_cond_init_instr(cond, __FUNCTION__)
#else
#define vm_cond_init _vm_cond_init
#endif

/**
 * This function destroys condition variable
 *
 * @param cond  pointer to condition variable
 *
 * @return 0 if success
 */
int _vm_cond_destroy(korp_cond *cond);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_cond_destroy_instr(korp_cond *cond, const char*func_name);
#define vm_cond_destroy(cond) vm_cond_destroy_instr(cond, __FUNCTION__)
#else
#define vm_cond_destroy _vm_cond_destroy
#endif

/**
 * This function will block on a condition varible.
 *
 * @param cond  pointer to condition variable
 * @param mutex  pointer to mutex to protect the condition variable
 *
 * @return 0 if success
 */
int _vm_cond_wait(korp_cond *cond, korp_mutex *mutex);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_cond_wait_instr(korp_cond *cond, korp_mutex *mutex, const char*func_name);
#define vm_cond_wait(cond, mutex) vm_cond_wait_instr(cond, mutex, __FUNCTION__)
#else
#define vm_cond_wait _vm_cond_wait
#endif

/**
 * This function will block on a condition varible or return if time specified passes.
 *
 * @param cond  pointer to condition variable
 * @param mutex  pointer to mutex to protect the condition variable
 * @param mills  milliseconds to wait
 *
 * @return 0 if success
 */
int _vm_cond_reltimedwait(korp_cond *cond, korp_mutex *mutex, int mills);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_cond_reltimedwait_instr(korp_cond *cond, korp_mutex *mutex, int mills, const char*func_name);
#define vm_cond_reltimedwait(cond, mutex, mills) vm_cond_reltimedwait_instr(cond, mutex, mills, __FUNCTION__)
#else
#define vm_cond_reltimedwait _vm_cond_reltimedwait
#endif

/**
 * This function signals the condition variable
 *
 * @param cond  condition variable
 *
 * @return 0 if success
 */
int _vm_cond_signal(korp_cond *cond);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_cond_signal_instr(korp_cond *cond, const char*func_name);
#define vm_cond_signal(cond) vm_cond_signal_instr(cond, __FUNCTION__)
#else
#define vm_cond_signal _vm_cond_signal
#endif

int _vm_cond_broadcast(korp_cond *cond);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_cond_broadcast_instr(korp_cond *cond, const char*func_name);
#define vm_cond_broadcast(cond) vm_cond_broadcast_instr(cond, __FUNCTION__)
#else
#define vm_cond_broadcast _vm_cond_broadcast
#endif

int _vm_thread_cancel(korp_tid thread);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_thread_cancel_instr(korp_tid thread, const char*func_name);
#define vm_thread_cancel(thread) vm_thread_cancel_instr(thread, __FUNCTION__)
#else
#define vm_thread_cancel _vm_thread_cancel
#endif

int _vm_thread_join(korp_tid thread, void **value_ptr, int mills);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_thread_join_instr(korp_tid thread, void **value_ptr, int mills, const char*func_name);
#define vm_thread_join(thread, value_ptr, mills) vm_thread_join_instr(thread, value_ptr, mills, __FUNCTION__)
#else
#define vm_thread_join _vm_thread_join
#endif

int _vm_thread_detach(korp_tid thread);
#ifdef _INSTRUMENT_TEST_ENABLED
int vm_thread_detach_instr(korp_tid thread, const char*func_name);
#define vm_thread_detach(thread) vm_thread_detach_instr(thread, __FUNCTION__)
#else
#define vm_thread_detach _vm_thread_detach
#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _BH_THREAD_H */
