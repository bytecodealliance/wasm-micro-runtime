/*
 * Copyright (c) 2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef RTTHREAD_PLATFORM_INTERNAL_H
#define RTTHREAD_PLATFORM_INTERNAL_H

#include <sys/ioctl.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <poll.h>
#if defined(RT_USING_PTHREADS)
#include <pthread.h>
#else
#include <rtthread.h>
#endif
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <ctype.h>
#include <dirent.h>
#include <assert.h>

#if defined(WASM_ENABLE_AOT)
#if defined(RTT_WAMR_BUILD_TARGET_THUMB)
#define BUILD_TARGET "thumbv4t"
#elif defined(RTT_WAMR_BUILD_TARGET_ARMV7)
#define BUILD_TARGET "armv7"
#elif defined(RTT_WAMR_BUILD_TARGET_ARMV6)
#define BUILD_TARGET "armv6"
#elif defined(RTT_WAMR_BUILD_TARGET_ARMV4)
#define BUILD_TARGET "armv4"
#elif defined(RTT_WAMR_BUILD_TARGET_X86_32)
#define BUILD_TARGET "X86_32"
#else
#error "unsupported aot platform."
#endif
#endif /* WASM_ENABLE_AOT */

/* Use rt-thread's definition as default */
#if 0 // defined(RT_USING_PTHREADS)
typedef pthread_t korp_tid;
typedef pthread_mutex_t korp_mutex;
typedef pthread_cond_t korp_cond;
typedef pthread_t korp_thread;
#else
typedef rt_thread_t korp_tid;
typedef struct rt_mutex korp_mutex;
typedef struct rt_thread korp_cond;
typedef struct rt_thread korp_thread;
#endif
typedef unsigned int korp_sem;

#if !defined(socklen_t) && !defined(SOCKLEN_T_DEFINED)
typedef uint32_t socklen_t;
#endif

#if !defined(SOL_SOCKET)
#define SOL_SOCKET 1
#endif

#if !defined(SO_TYPE)
#define SO_TYPE 3
#endif

#if !defined(SOCK_DGRAM)
#define SOCK_DGRAM 2
#endif

#if !defined(SOCK_STREAM)
#define SOCK_STREAM 1
#endif

#if !defined(UTIME_NOW)
#define UTIME_NOW -2L
#endif

#if !defined(UTIME_OMIT)
#define UTIME_OMIT -1L
#endif

#if !defined(AT_SYMLINK_NOFOLLOW)
#define AT_SYMLINK_NOFOLLOW 2
#endif

#if !defined(AT_SYMLINK_FOLLOW)
#define AT_SYMLINK_FOLLOW 4
#endif

#if !defined(AT_REMOVEDIR)
#define AT_REMOVEDIR 8
#endif

#define DT_BLK 0x06
#define DT_CHR 0x02
#define DT_LNK 0x0A

#define PTHREAD_STACK_MIN 1024
#define BH_THREAD_DEFAULT_PRIORITY 30

/* korp_rwlock is used in platform_api_extension.h,
   we just define the type to make the compiler happy */
typedef struct {
    int dummy;
} korp_rwlock;

typedef rt_uint8_t uint8_t;
typedef rt_int8_t int8_t;
typedef rt_uint16_t uint16_t;
typedef rt_int16_t int16_t;
typedef rt_uint64_t uint64_t;
typedef rt_int64_t int64_t;

/* The below types are used in platform_api_extension.h,
   we just define them to make the compiler happy */
typedef int os_file_handle;
typedef void *os_dir_stream;
typedef int os_raw_file_handle;

static inline os_file_handle
os_get_invalid_handle(void)
{
    return -1;
}

#endif /* RTTHREAD_PLATFORM_INTERNAL_H */
