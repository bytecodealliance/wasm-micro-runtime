/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _PLATFORM_INTERNAL_H
#define _PLATFORM_INTERNAL_H

#include <aos/kernel.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BH_PLATFORM_ALIOS_THINGS
#define BH_PLATFORM_ALIOS_THINGS
#endif

#define BH_APPLET_PRESERVED_STACK_SIZE (2 * BH_KB)

/* Default thread priority */
#define BH_THREAD_DEFAULT_PRIORITY 30

typedef aos_task_t korp_thread;
typedef korp_thread *korp_tid;
typedef aos_task_t *aos_tid_t;
typedef aos_mutex_t korp_mutex;
typedef aos_sem_t korp_sem;

/* korp_rwlock is used in platform_api_extension.h,
   we just define the type to make the compiler happy */
typedef struct {
    int dummy;
} korp_rwlock;

struct os_thread_wait_node;
typedef struct os_thread_wait_node *os_thread_wait_list;
typedef struct korp_cond {
    aos_mutex_t wait_list_lock;
    os_thread_wait_list thread_wait_list;
} korp_cond;

#define os_printf printf
#define os_vprintf vprintf

/* clang-format off */
/* math functions which are not provided by os*/
double sqrt(double x);
double floor(double x);
double ceil(double x);
double fmin(double x, double y);
double fmax(double x, double y);
double rint(double x);
double fabs(double x);
double trunc(double x);
float sqrtf(float x);
float floorf(float x);
float ceilf(float x);
float fminf(float x, float y);
float fmaxf(float x, float y);
float rintf(float x);
float fabsf(float x);
float truncf(float x);
int isnan_double(double x);
int isnan_float(float x);
int signbit_double(double x);
int signbit_float(float x);
#define isnan(x) (sizeof(x) == sizeof(double) ? isnan_double((double)x) : isnan_float(x))
#define signbit(x) (sizeof(x) == sizeof(double) ? signbit_double((double)x) : signbit_float(x))
/* clang-format on */

/* The below types are used in platform_api_extension.h,
   we just define them to make the compiler happy */
typedef int os_file_handle;
typedef void *os_dir_stream;
typedef int os_raw_file_handle;
typedef int os_poll_file_handle;
typedef unsigned int os_nfds_t;
typedef int os_timespec;

static inline os_file_handle
os_get_invalid_handle(void)
{
    return -1;
}

#endif /* end of _BH_PLATFORM_H */
