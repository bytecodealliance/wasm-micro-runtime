/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
#include <stdlib.h>
#include <string.h>

#ifndef BH_PLATFORM_ALIOS_THINGS
#define BH_PLATFORM_ALIOS_THINGS
#endif

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

#define bh_printf printf

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
float floorf(float x);
float ceilf(float x);
float fminf(float x, float y);
float fmaxf(float x, float y);
float rintf(float x);
float truncf(float x);
int signbit(double x);
int isnan(double x);

int bh_platform_init();

/* MMAP mode */
enum {
    MMAP_PROT_NONE = 0,
    MMAP_PROT_READ = 1,
    MMAP_PROT_WRITE = 2,
    MMAP_PROT_EXEC = 4
};

/* MMAP flags */
enum {
    MMAP_MAP_NONE = 0,
    /* Put the mapping into 0 to 2 G, supported only on x86_64 */
    MMAP_MAP_32BIT = 1,
    /* Don't interpret addr as a hint: place the mapping at exactly
       that address. */
    MMAP_MAP_FIXED = 2
};

void *bh_mmap(void *hint, unsigned int size, int prot, int flags);
void bh_munmap(void *addr, uint32 size);
int bh_mprotect(void *addr, uint32 size, int prot);

#endif /* end of _BH_PLATFORM_H */

