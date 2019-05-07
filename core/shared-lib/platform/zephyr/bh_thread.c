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

typedef struct bh_thread_wait_node {
    struct k_sem sem;
    bh_thread_wait_list next;
} bh_thread_wait_node;

typedef struct bh_thread_data {
    /* Next thread data */
    struct bh_thread_data *next;
    /* Zephyr thread handle */
    korp_tid tid;
    /* Jeff thread local root */
    void *tlr;
    /* Lock for waiting list */
    struct k_mutex wait_list_lock;
    /* Waiting list of other threads who are joining this thread */
    bh_thread_wait_list thread_wait_list;
    /* Thread stack size */
    unsigned stack_size;
    /* Thread stack */
    char stack[1];
} bh_thread_data;

typedef struct bh_thread_obj {
    struct k_thread thread;
    /* Whether the thread is terminated and this thread object is to
     be freed in the future. */
    bool to_be_freed;
    struct bh_thread_obj *next;
} bh_thread_obj;

static bool is_thread_sys_inited = false;

/* Thread data of supervisor thread */
static bh_thread_data supervisor_thread_data;

/* Lock for thread data list */
static struct k_mutex thread_data_lock;

/* Thread data list */
static bh_thread_data *thread_data_list = NULL;

/* Lock for thread object list */
static struct k_mutex thread_obj_lock;

/* Thread object list */
static bh_thread_obj *thread_obj_list = NULL;

static void thread_data_list_add(bh_thread_data *thread_data)
{
    k_mutex_lock(&thread_data_lock, K_FOREVER);
    if (!thread_data_list)
        thread_data_list = thread_data;
    else {
        /* If already in list, just return */
        bh_thread_data *p = thread_data_list;
        while (p) {
            if (p == thread_data) {
                k_mutex_unlock(&thread_data_lock);
                return;
            }
            p = p->next;
        }

        /* Set as head of list */
        thread_data->next = thread_data_list;
        thread_data_list = thread_data;
    }
    k_mutex_unlock(&thread_data_lock);
}

static void thread_data_list_remove(bh_thread_data *thread_data)
{
    k_mutex_lock(&thread_data_lock, K_FOREVER);
    if (thread_data_list) {
        if (thread_data_list == thread_data)
            thread_data_list = thread_data_list->next;
        else {
            /* Search and remove it from list */
            bh_thread_data *p = thread_data_list;
            while (p && p->next != thread_data)
                p = p->next;
            if (p && p->next == thread_data)
                p->next = p->next->next;
        }
    }
    k_mutex_unlock(&thread_data_lock);
}

static bh_thread_data *
thread_data_list_lookup(k_tid_t tid)
{
    k_mutex_lock(&thread_data_lock, K_FOREVER);
    if (thread_data_list) {
        bh_thread_data *p = thread_data_list;
        while (p) {
            if (p->tid == tid) {
                /* Found */
                k_mutex_unlock(&thread_data_lock);
                return p;
            }
            p = p->next;
        }
    }
    k_mutex_unlock(&thread_data_lock);
    return NULL;
}

static void thread_obj_list_add(bh_thread_obj *thread_obj)
{
    k_mutex_lock(&thread_obj_lock, K_FOREVER);
    if (!thread_obj_list)
        thread_obj_list = thread_obj;
    else {
        /* Set as head of list */
        thread_obj->next = thread_obj_list;
        thread_obj_list = thread_obj;
    }
    k_mutex_unlock(&thread_obj_lock);
}

static void thread_obj_list_reclaim()
{
    bh_thread_obj *p, *p_prev;
    k_mutex_lock(&thread_obj_lock, K_FOREVER);
    p_prev = NULL;
    p = thread_obj_list;
    while (p) {
        if (p->to_be_freed) {
            if (p_prev == NULL) { /* p is the head of list */
                thread_obj_list = p->next;
                bh_free(p);
                p = thread_obj_list;
            } else { /* p is not the head of list */
                p_prev->next = p->next;
                bh_free(p);
                p = p_prev->next;
            }
        } else {
            p_prev = p;
            p = p->next;
        }
    }
    k_mutex_unlock(&thread_obj_lock);
}

int _vm_thread_sys_init()
{
    if (is_thread_sys_inited)
        return BHT_OK;

    k_mutex_init(&thread_data_lock);
    k_mutex_init(&thread_obj_lock);

    /* Initialize supervisor thread data */
    memset(&supervisor_thread_data, 0, sizeof(supervisor_thread_data));
    supervisor_thread_data.tid = k_current_get();
    /* Set as head of thread data list */
    thread_data_list = &supervisor_thread_data;

    is_thread_sys_inited = true;
    return BHT_OK;
}

void vm_thread_sys_destroy(void)
{
    if (is_thread_sys_inited) {
        is_thread_sys_inited = false;
    }
}

static bh_thread_data *
thread_data_current()
{
    k_tid_t tid = k_current_get();
    return thread_data_list_lookup(tid);
}

static void vm_thread_cleanup(void)
{
    bh_thread_data *thread_data = thread_data_current();

    bh_assert(thread_data != NULL);
    k_mutex_lock(&thread_data->wait_list_lock, K_FOREVER);
    if (thread_data->thread_wait_list) {
        /* Signal each joining thread */
        bh_thread_wait_list head = thread_data->thread_wait_list;
        while (head) {
            bh_thread_wait_list next = head->next;
            k_sem_give(&head->sem);
            bh_free(head);
            head = next;
        }
        thread_data->thread_wait_list = NULL;
    }
    k_mutex_unlock(&thread_data->wait_list_lock);

    thread_data_list_remove(thread_data);
    /* Set flag to true for the next thread creating to
     free the thread object */
    ((bh_thread_obj*) thread_data->tid)->to_be_freed = true;
    bh_free(thread_data);
}

static void vm_thread_wrapper(void *start, void *arg, void *thread_data)
{
    /* Set thread custom data */
    ((bh_thread_data*) thread_data)->tid = k_current_get();
    thread_data_list_add(thread_data);

    ((thread_start_routine_t) start)(arg);
    vm_thread_cleanup();
}

int _vm_thread_create(korp_tid *p_tid, thread_start_routine_t start, void *arg,
        unsigned int stack_size)
{
    return _vm_thread_create_with_prio(p_tid, start, arg, stack_size,
    BH_THREAD_DEFAULT_PRIORITY);
}

int _vm_thread_create_with_prio(korp_tid *p_tid, thread_start_routine_t start,
        void *arg, unsigned int stack_size, int prio)
{
    korp_tid tid;
    bh_thread_data *thread_data;
    unsigned thread_data_size;

    if (!p_tid || !stack_size)
        return BHT_ERROR;

    /* Free the thread objects of terminated threads */
    thread_obj_list_reclaim();

    /* Create and initialize thread object */
    if (!(tid = bh_malloc(sizeof(bh_thread_obj))))
        return BHT_ERROR;

    memset(tid, 0, sizeof(bh_thread_obj));

    /* Create and initialize thread data */
    thread_data_size = offsetof(bh_thread_data, stack) + stack_size;
    if (!(thread_data = bh_malloc(thread_data_size))) {
        bh_free(tid);
        return BHT_ERROR;
    }

    memset(thread_data, 0, thread_data_size);
    k_mutex_init(&thread_data->wait_list_lock);
    thread_data->stack_size = stack_size;
    thread_data->tid = tid;

    /* Create the thread */
    if (!((tid = k_thread_create(tid, (k_thread_stack_t *) thread_data->stack,
            stack_size, vm_thread_wrapper, start, arg, thread_data, prio, 0,
            K_NO_WAIT)))) {
        bh_free(tid);
        bh_free(thread_data);
        return BHT_ERROR;
    }

    bh_assert(tid == thread_data->tid);

    /* Set thread custom data */
    thread_data_list_add(thread_data);
    thread_obj_list_add((bh_thread_obj*) tid);
    *p_tid = tid;
    return BHT_OK;
}

korp_tid _vm_self_thread()
{
    return (korp_tid) k_current_get();
}

void vm_thread_exit(void * code)
{
    (void) code;
    korp_tid self = vm_self_thread();
    vm_thread_cleanup();
    k_thread_abort((k_tid_t) self);
}

int _vm_thread_cancel(korp_tid thread)
{
    k_thread_abort((k_tid_t) thread);
    return 0;
}

int _vm_thread_join(korp_tid thread, void **value_ptr, int mills)
{
    (void) value_ptr;
    bh_thread_data *thread_data;
    bh_thread_wait_node *node;

    /* Create wait node and append it to wait list */
    if (!(node = bh_malloc(sizeof(bh_thread_wait_node))))
        return BHT_ERROR;

    k_sem_init(&node->sem, 0, 1);
    node->next = NULL;

    /* Get thread data */
    thread_data = thread_data_list_lookup(thread);
    bh_assert(thread_data != NULL);

    k_mutex_lock(&thread_data->wait_list_lock, K_FOREVER);
    if (!thread_data->thread_wait_list)
        thread_data->thread_wait_list = node;
    else {
        /* Add to end of waiting list */
        bh_thread_wait_node *p = thread_data->thread_wait_list;
        while (p->next)
            p = p->next;
        p->next = node;
    }
    k_mutex_unlock(&thread_data->wait_list_lock);

    /* Wait the sem */
    k_sem_take(&node->sem, mills);

    /* Wait some time for the thread to be actually terminated */
    k_sleep(100);

    return BHT_OK;
}

int _vm_thread_detach(korp_tid thread)
{
    (void) thread;
    return BHT_OK;
}

void *_vm_tls_get(unsigned idx)
{
    (void) idx;
    bh_thread_data *thread_data;

    bh_assert(idx == 0);
    thread_data = thread_data_current();

    return thread_data ? thread_data->tlr : NULL;
}

int _vm_tls_put(unsigned idx, void * tls)
{
    bh_thread_data *thread_data;

    (void) idx;
    bh_assert(idx == 0);
    thread_data = thread_data_current();
    bh_assert(thread_data != NULL);

    thread_data->tlr = tls;
    return BHT_OK;
}

int _vm_mutex_init(korp_mutex *mutex)
{
    (void) mutex;
    k_mutex_init(mutex);
    return BHT_OK;
}

int _vm_recursive_mutex_init(korp_mutex *mutex)
{
    k_mutex_init(mutex);
    return BHT_OK;
}

int _vm_mutex_destroy(korp_mutex *mutex)
{
    (void) mutex;
    return BHT_OK;
}

void vm_mutex_lock(korp_mutex *mutex)
{
    k_mutex_lock(mutex, K_FOREVER);
}

int vm_mutex_trylock(korp_mutex *mutex)
{
    return k_mutex_lock(mutex, K_NO_WAIT);
}

void vm_mutex_unlock(korp_mutex *mutex)
{
    k_mutex_unlock(mutex);
}

int _vm_sem_init(korp_sem* sem, unsigned int c)
{
    k_sem_init(sem, 0, c);
    return BHT_OK;
}

int _vm_sem_destroy(korp_sem *sem)
{
    (void) sem;
    return BHT_OK;
}

int _vm_sem_wait(korp_sem *sem)
{
    return k_sem_take(sem, K_FOREVER);
}

int _vm_sem_reltimedwait(korp_sem *sem, int mills)
{
    return k_sem_take(sem, mills);
}

int _vm_sem_post(korp_sem *sem)
{
    k_sem_give(sem);
    return BHT_OK;
}

int _vm_cond_init(korp_cond *cond)
{
    k_mutex_init(&cond->wait_list_lock);
    cond->thread_wait_list = NULL;
    return BHT_OK;
}

int _vm_cond_destroy(korp_cond *cond)
{
    (void) cond;
    return BHT_OK;
}

static int _vm_cond_wait_internal(korp_cond *cond, korp_mutex *mutex,
        bool timed, int mills)
{
    bh_thread_wait_node *node;

    /* Create wait node and append it to wait list */
    if (!(node = bh_malloc(sizeof(bh_thread_wait_node))))
        return BHT_ERROR;

    k_sem_init(&node->sem, 0, 1);
    node->next = NULL;

    k_mutex_lock(&cond->wait_list_lock, K_FOREVER);
    if (!cond->thread_wait_list)
        cond->thread_wait_list = node;
    else {
        /* Add to end of wait list */
        bh_thread_wait_node *p = cond->thread_wait_list;
        while (p->next)
            p = p->next;
        p->next = node;
    }
    k_mutex_unlock(&cond->wait_list_lock);

    /* Unlock mutex, wait sem and lock mutex again */
    k_mutex_unlock(mutex);
    k_sem_take(&node->sem, timed ? mills : K_FOREVER);
    k_mutex_lock(mutex, K_FOREVER);

    /* Remove wait node from wait list */
    k_mutex_lock(&cond->wait_list_lock, K_FOREVER);
    if (cond->thread_wait_list == node)
        cond->thread_wait_list = node->next;
    else {
        /* Remove from the wait list */
        bh_thread_wait_node *p = cond->thread_wait_list;
        while (p->next != node)
            p = p->next;
        p->next = node->next;
    }
    bh_free(node);
    k_mutex_unlock(&cond->wait_list_lock);

    return BHT_OK;
}

int _vm_cond_wait(korp_cond *cond, korp_mutex *mutex)
{
    return _vm_cond_wait_internal(cond, mutex, false, 0);
}

int _vm_cond_reltimedwait(korp_cond *cond, korp_mutex *mutex, int mills)
{
    return _vm_cond_wait_internal(cond, mutex, true, mills);
}

int _vm_cond_signal(korp_cond *cond)
{
    /* Signal the head wait node of wait list */
    k_mutex_lock(&cond->wait_list_lock, K_FOREVER);
    if (cond->thread_wait_list)
        k_sem_give(&cond->thread_wait_list->sem);
    k_mutex_unlock(&cond->wait_list_lock);

    return BHT_OK;
}

int _vm_cond_broadcast(korp_cond *cond)
{
    /* Signal each wait node of wait list */
    k_mutex_lock(&cond->wait_list_lock, K_FOREVER);
    if (cond->thread_wait_list) {
        bh_thread_wait_node *p = cond->thread_wait_list;
        while (p) {
            k_sem_give(&p->sem);
            p = p->next;
        }
    }
    k_mutex_unlock(&cond->wait_list_lock);

    return BHT_OK;
}

