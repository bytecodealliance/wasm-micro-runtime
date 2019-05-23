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

#ifndef _WASM_PLATFORM_H
#define _WASM_PLATFORM_H

#include "wasm_config.h"
#include "wasm_types.h"
#include <aos/kernel.h>
#include <inttypes.h>

#include <stdbool.h>
typedef uint64_t uint64;
typedef int64_t int64;
typedef float float32;
typedef double float64;

#ifndef NULL
#  define NULL ((void*) 0)
#endif

#define WASM_PLATFORM "AliOS"
#define __ALIOS__ 1

#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

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

typedef aos_task_t korp_thread;
typedef korp_thread *korp_tid;
typedef aos_mutex_t korp_mutex;

int wasm_platform_init();

extern bool is_little_endian;

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

#ifdef __cplusplus
extern "C" {
#endif

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

void*
wasm_dlsym(void *handle, const char *symbol);

#ifdef __cplusplus
}
#endif

#endif
