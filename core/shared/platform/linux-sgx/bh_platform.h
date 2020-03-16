/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _BH_PLATFORM_H
#define _BH_PLATFORM_H

#include "bh_config.h"
#include "bh_types.h"
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <sgx_thread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*bh_print_function_t)(const char* message);
void bh_set_print_function(bh_print_function_t pf);

extern int bh_printf_sgx(const char *message, ...);
extern int bh_vprintf_sgx(const char * format, va_list arg);

typedef uint64_t uint64;
typedef int64_t int64;

#ifndef BH_PLATFORM_LINUX_SGX
#define BH_PLATFORM_LINUX_SGX
#endif

/* NEED qsort */

#define _STACK_SIZE_ADJUSTMENT (32 * 1024)

/* Stack size of applet threads's native part.  */
#define BH_APPLET_PRESERVED_STACK_SIZE      (8 * 1024 + _STACK_SIZE_ADJUSTMENT)

/* Default thread priority */
#define BH_THREAD_DEFAULT_PRIORITY 0

#define BH_ROUTINE_MODIFIER

#define BHT_TIMEDOUT ETIMEDOUT

#define INVALID_THREAD_ID 0xFFffFFff

typedef int korp_sem;
typedef void* (*thread_start_routine_t)(void*);
typedef sgx_thread_mutex_t korp_mutex;
typedef sgx_thread_t korp_tid;
typedef sgx_thread_t korp_thread;
typedef sgx_thread_cond_t korp_cond;

void *os_malloc(unsigned size);
void *os_realloc(void *ptr, unsigned size);
void os_free(void *ptr);

#define bh_printf bh_printf_sgx

int snprintf(char *buffer, size_t count, const char *format, ...);
int strncasecmp(const char *s1, const char *s2, size_t n);
double fmod(double x, double y);
float fmodf(float x, float y);
double sqrt(double x);

#define BH_WAIT_FOREVER 0xFFFFFFFF

#ifndef NULL
#  define NULL ((void*) 0)
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

#define bh_assert assert

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
#define bh_dcache_flush() (void)0

#ifdef __cplusplus
}
#endif

#endif
