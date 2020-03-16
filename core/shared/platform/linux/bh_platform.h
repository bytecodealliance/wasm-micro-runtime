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
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t uint64;
typedef int64_t int64;

extern void DEBUGME(void);

#define DIE do{bh_debug("Die here\n\n\n\n\n\n\n\n\n\n\n\n\n\n"); DEBUGME(void); while(1);}while(0)

#ifndef BH_PLATFORM_LINUX
#define BH_PLATFORM_LINUX
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

typedef pthread_t korp_tid;
typedef pthread_mutex_t korp_mutex;
typedef sem_t korp_sem;
typedef pthread_cond_t korp_cond;
typedef pthread_t korp_thread;
typedef void* (*thread_start_routine_t)(void*);

void *os_malloc(unsigned size);
void *os_realloc(void *ptr, unsigned size);
void os_free(void *ptr);

#define bh_printf printf

int snprintf(char *buffer, size_t count, const char *format, ...);
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

char *bh_read_file_to_buffer(const char *filename, uint32 *ret_size);

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
