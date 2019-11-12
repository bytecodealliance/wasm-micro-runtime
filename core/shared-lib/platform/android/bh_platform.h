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
typedef uint64_t uint64;
typedef int64_t int64;

extern void DEBUGME(void);

#define DIE do{bh_debug("Die here\n\n\n\n\n\n\n\n\n\n\n\n\n\n"); DEBUGME(void); while(1);}while(0)

#define BH_PLATFORM "Linux"

/* NEED qsort */

#include <stdarg.h>
#include <ctype.h>
#include <pthread.h>
#include <limits.h>
#include <semaphore.h>
#include <errno.h>

#define _STACK_SIZE_ADJUSTMENT (32 * 1024)

/* Stack size of applet manager thread.  */
#define BH_APPLET_MANAGER_THREAD_STACK_SIZE (8 * 1024 + _STACK_SIZE_ADJUSTMENT)

/* Stack size of HMC thread.  */
#define BH_HMC_THREAD_STACK_SIZE            (4 * 1024 + _STACK_SIZE_ADJUSTMENT)

/* Stack size of watchdog thread.  */
#define BH_WATCHDOG_THREAD_SIZE             (4 * 1024 + _STACK_SIZE_ADJUSTMENT)

/* Stack size of applet threads's native part.  */
#define BH_APPLET_PRESERVED_STACK_SIZE      (8 * 1024 + _STACK_SIZE_ADJUSTMENT)

/* Stack size of remote invoke listen thread.  */
#define BH_REMOTE_INVOKE_THREAD_STACK_SIZE  (4 * 1024 + _STACK_SIZE_ADJUSTMENT)

/* Stack size of remote post listen thread.  */
#define BH_REMOTE_POST_THREAD_STACK_SIZE    (4 * 1024 + _STACK_SIZE_ADJUSTMENT)

/* Maximal recursion depth of interpreter.  */
#define BH_MAX_INTERP_RECURSION_DEPTH       8

#define BH_ROUTINE_MODIFIER
#define BHT_TIMEDOUT ETIMEDOUT

#define INVALID_THREAD_ID 0xFFffFFff
#define INVALID_SEM_ID SEM_FAILED

typedef pthread_t korp_tid;
typedef pthread_mutex_t korp_mutex;
typedef sem_t korp_sem;
typedef pthread_cond_t korp_cond;
typedef void* (*thread_start_routine_t)(void*);

#include <time.h>
#include <string.h>

/* The following operations declared in string.h may be defined as
 macros on Linux, so don't declare them as functions here.  */
/* memset */
/* memcpy */
/* memmove */

/* #include <stdio.h> */

/* Unit test framework is based on C++, where the declaration of
 snprintf is different.  */
#ifndef __cplusplus
int snprintf(char *buffer, size_t count, const char *format, ...);
#endif

/* #include <math.h> */

double fmod(double x, double y);
float fmodf(float x, float y);

/* Definitions for applet debugging */
#define APPLET_DEBUG_LISTEN_PORT 8000
#define BH_SOCKET_INVALID_SOCK -1
#define BH_WAIT_FOREVER 0xFFFFFFFF
typedef int bh_socket_t;

#ifndef NULL
#  define NULL ((void*) 0)
#endif

extern int b_memcpy_s(void * s1, unsigned int s1max, const void * s2,
        unsigned int n);
extern int strcat_s(char * s1, size_t s1max, const char * s2);
extern int strcpy_s(char * s1, size_t s1max, const char * s2);
#include <stdio.h>
extern int fopen_s(FILE ** pFile, const char *filename, const char *mode);

#endif
