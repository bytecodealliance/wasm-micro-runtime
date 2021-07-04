/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _PLATFORM_COMMON_H
#define _PLATFORM_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "platform_internal.h"
#include "../../../config.h"

#define BH_MAX_THREAD 32

#define BHT_ERROR (-1)
#define BHT_TIMED_OUT (1)
#define BHT_OK (0)

#define BHT_WAIT_FOREVER ((uint64)-1LL)

#define BH_KB (1024)
#define BH_MB ((BH_KB)*1024)
#define BH_GB ((BH_MB)*1024)

#ifndef BH_MALLOC
#define BH_MALLOC os_malloc
#endif

#ifndef BH_FREE
#define BH_FREE os_free
#endif

#ifndef BH_TIME_T_MAX
#define BH_TIME_T_MAX LONG_MAX
#endif

#if defined(_MSC_BUILD)
#if defined(COMPILING_WASM_RUNTIME_API)
__declspec(dllexport) void *BH_MALLOC(unsigned int size);
__declspec(dllexport) void BH_FREE(void *ptr);
#else
__declspec(dllimport) void* BH_MALLOC(unsigned int size);
__declspec(dllimport) void BH_FREE(void* ptr);
#endif
#else
void *BH_MALLOC(unsigned int size);
void BH_FREE(void *ptr);
#endif

#if defined(BH_VPRINTF)
#if defined(MSVC)
__declspec(dllimport) int BH_VPRINTF(const char *format, va_list ap);
#else
int BH_VPRINTF(const char *format, va_list ap);
#endif
#endif

#ifndef NULL
#define NULL (void*)0
#endif

#ifndef __cplusplus

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef inline
#define inline __inline
#endif

#endif

/* Return the offset of the given field in the given type */
#ifndef offsetof
#define offsetof(Type, field) ((size_t)(&((Type *)0)->field))
#endif

typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef float float32;
typedef double float64;
typedef uint64_t uint64;
typedef int64_t int64;

typedef void* (*thread_start_routine_t)(void*);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _PLATFORM_COMMON_H */
