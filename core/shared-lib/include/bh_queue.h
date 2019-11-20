/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _BH_QUEUE_H
#define _BH_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bh_types.h" /*For bool type*/
#include "bh_platform.h"

struct _bh_queue_node;
typedef struct _bh_queue_node * bh_message_t;
struct bh_queue;
typedef struct bh_queue bh_queue;

typedef void (*bh_queue_handle_msg_callback)(void *message, void *arg);

#define bh_queue_malloc bh_malloc
#define bh_queue_free bh_free

#define bh_queue_mutex korp_mutex
#define bh_queue_sem korp_sem
#define bh_queue_cond korp_cond

#define bh_queue_mutex_init vm_mutex_init
#define bh_queue_mutex_destroy vm_mutex_destroy
#define bh_queue_mutex_lock vm_mutex_lock
#define bh_queue_mutex_unlock vm_mutex_unlock

#define bh_queue_sem_init vm_sem_init
#define bh_queue_sem_destroy vm_sem_destroy
#define bh_queue_sem_wait vm_sem_wait
#define bh_queue_sem_reltimedwait vm_sem_reltimedwait
#define bh_queue_sem_post vm_sem_post

#define bh_queue_cond_init vm_cond_init
#define bh_queue_cond_destroy vm_cond_destroy
#define bh_queue_cond_wait vm_cond_wait
#define bh_queue_cond_timedwait vm_cond_reltimedwait
#define bh_queue_cond_signal vm_cond_signal
#define bh_queue_cond_broadcast vm_cond_broadcast

typedef void (*bh_msg_cleaner)(void *msg);

bh_queue *
bh_queue_create();

void
bh_queue_destroy(bh_queue *queue);

char * bh_message_payload(bh_message_t message);
int bh_message_payload_len(bh_message_t message);
int bh_message_type(bh_message_t message);

bh_message_t bh_new_msg(unsigned short tag, void *body, unsigned int len,
        void * handler);
void bh_free_msg(bh_message_t msg);
bool bh_post_msg(bh_queue *queue, unsigned short tag, void *body,
        unsigned int len);
bool bh_post_msg2(bh_queue *queue, bh_message_t msg);

bh_message_t bh_get_msg(bh_queue *queue, int timeout);

unsigned
bh_queue_get_message_count(bh_queue *queue);

void
bh_queue_enter_loop_run(bh_queue *queue,
                        bh_queue_handle_msg_callback handle_cb,
                        void *arg);
void
bh_queue_exit_loop_run(bh_queue *queue);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _BH_QUEUE_H */

