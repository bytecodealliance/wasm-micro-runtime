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
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <pthread.h>
#include <limits.h>
#include <semaphore.h>
#include <errno.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t uint64;
typedef int64_t int64;

extern void DEBUGME(void);

#define DIE do{bh_debug("Die here\n\n\n\n\n\n\n\n\n\n\n\n\n\n"); DEBUGME(void); while(1);}while(0)

#define BH_PLATFORM "VxWorks"

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

#define wa_malloc bh_malloc
#define wa_free bh_free
#define wa_strdup bh_strdup

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

int b_memcpy_s(void * s1, unsigned int s1max, const void * s2,
               unsigned int n);
int b_strcat_s(char * s1, size_t s1max, const char * s2);
int b_strcpy_s(char * s1, size_t s1max, const char * s2);

char *bh_read_file_to_buffer(const char *filename, int *ret_size);

char *bh_strdup(const char *s);

int bh_platform_init();

#ifdef __cplusplus
}
#endif

#endif
