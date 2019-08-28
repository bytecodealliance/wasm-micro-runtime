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
#include <aos/kernel.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Platform name */
#define BH_PLATFORM "AliOS-Things"

#define BH_APPLET_PRESERVED_STACK_SIZE (2 * BH_KB)

/* Default thread priority */
#define BH_THREAD_DEFAULT_PRIORITY 30

#define BH_ROUTINE_MODIFIER

/* Invalid thread tid */
#define INVALID_THREAD_ID NULL

#define BH_WAIT_FOREVER AOS_WAIT_FOREVER

typedef uint64_t uint64;
typedef int64_t int64;

#define wa_malloc bh_malloc
#define wa_free bh_free
#define wa_strdup bh_strdup

typedef aos_task_t korp_thread;
typedef korp_thread *korp_tid;
typedef aos_task_t *aos_tid_t;
typedef aos_mutex_t korp_mutex;
typedef aos_sem_t korp_sem;

struct bh_thread_wait_node;
typedef struct bh_thread_wait_node *bh_thread_wait_list;
typedef struct korp_cond {
  aos_mutex_t wait_list_lock;
  bh_thread_wait_list thread_wait_list;
} korp_cond;

typedef void* (*thread_start_routine_t)(void*);

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

extern void bh_assert_internal(int v, const char *file_name, int line_number, const char *expr_string);
#define bh_assert(expr) bh_assert_internal((int)(expr), __FILE__, __LINE__, # expr)

extern int b_memcpy_s(void * s1, unsigned int s1max, const void * s2, unsigned int n);
extern int b_strcat_s(char * s1, size_t s1max, const char * s2);
extern int b_strcpy_s(char * s1, size_t s1max, const char * s2);

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

#endif /* end of _BH_PLATFORM_H */

