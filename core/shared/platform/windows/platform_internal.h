/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _PLATFORM_INTERNAL_H
#define _PLATFORM_INTERNAL_H

#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <sys/timeb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BH_PLATFORM_WINDOWS
#define BH_PLATFORM_WINDOWS
#endif

/* Stack size of applet threads's native part.  */
#define BH_APPLET_PRESERVED_STACK_SIZE (32 * 1024)

/* Default thread priority */
#define BH_THREAD_DEFAULT_PRIORITY 0

typedef void *korp_tid;
typedef void *korp_mutex;
typedef void *korp_sem;
typedef void *korp_thread;

typedef struct {
    korp_sem s;
    unsigned int waiting_count;
} korp_cond;

#define os_printf printf
#define os_vprintf vprintf

static inline size_t
getpagesize()
{
    SYSTEM_INFO S;
    GetNativeSystemInfo(&S);
    return S.dwPageSize;
}

#ifdef __cplusplus
}
#endif

#endif /* end of _PLATFORM_INTERNAL_H */

