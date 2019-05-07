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

#ifndef _BH_PLATFORM_H
#define _BH_PLATFORM_H

#include "bh_config.h"
#include "bh_types.h"
#include "bh_memory.h"
#include <autoconf.h>
#include <zephyr.h>
#include <kernel.h>
#include <misc/printk.h>
#include <inttypes.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifndef CONFIG_NET_BUF_USER_DATA_SIZE
#define CONFIG_NET_BUF_USER_DATA_SIZE 0
#endif
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>

/* Platform name */
#define BH_PLATFORM "Zephyr"

#define BH_APPLET_PRESERVED_STACK_SIZE (2 * BH_KB)

/* Default thread priority */
#define BH_THREAD_DEFAULT_PRIORITY 7

#define BH_ROUTINE_MODIFIER

/* Invalid thread tid */
#define INVALID_THREAD_ID NULL

#define INVALID_SEM_ID SEM_FAILED
#define BH_WAIT_FOREVER K_FOREVER

typedef uint64_t uint64;
typedef int64_t int64;

typedef struct k_thread korp_thread;
typedef korp_thread *korp_tid;
typedef struct k_mutex korp_mutex;
typedef struct k_sem korp_sem;

#define wa_malloc bh_malloc
#define wa_free bh_free
#define wa_strdup bh_strdup

struct bh_thread_wait_node;
typedef struct bh_thread_wait_node *bh_thread_wait_list;
typedef struct korp_cond {
    struct k_mutex wait_list_lock;
    bh_thread_wait_list thread_wait_list;
} korp_cond;

typedef void* (*thread_start_routine_t)(void*);

#define wa_malloc bh_malloc
#define wa_free bh_free

/* Unit test framework is based on C++, where the declaration of
 snprintf is different.  */
#ifndef __cplusplus
int snprintf(char *buffer, size_t count, const char *format, ...);
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#define bh_assert(x) \
    do { \
        if (!(x)) { \
            printk("bh_assert(%s, %d)\n", __func__, __LINE__);\
        } \
    } while (0)

extern int b_memcpy_s(void * s1, unsigned int s1max, const void * s2,
        unsigned int n);
extern int b_strcat_s(char * s1, size_t s1max, const char * s2);
extern int b_strcpy_s(char * s1, size_t s1max, const char * s2);

#endif
