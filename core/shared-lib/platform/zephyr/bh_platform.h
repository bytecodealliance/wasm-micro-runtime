/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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

/**
 * Return the offset of the given field in the given type.
 *
 * @param Type the type containing the filed
 * @param field the field in the type
 *
 * @return the offset of field in Type
 */
#ifndef offsetof
#define offsetof(Type, field) ((size_t)(&((Type *)0)->field))
#endif

#define bh_assert(x) \
    do { \
        if (!(x)) { \
            printk("bh_assert(%s, %d)\n", __func__, __LINE__);\
        } \
    } while (0)

int b_memcpy_s(void * s1, unsigned int s1max, const void * s2,
               unsigned int n);
int b_strcat_s(char * s1, size_t s1max, const char * s2);
int b_strcpy_s(char * s1, size_t s1max, const char * s2);

/* math functions */
double sqrt(double x);
double floor(double x);
double ceil(double x);
double fmin(double x, double y);
double fmax(double x, double y);
double rint(double x);
double fabs(double x);
double trunc(double x);
int signbit(double x);
int isnan(double x);

int bh_platform_init();

#endif
