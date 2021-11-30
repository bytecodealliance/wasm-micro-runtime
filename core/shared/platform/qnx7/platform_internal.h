/*
 * Copyright (C) 2020 Quux Oy
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _PLATFORM_INTERNAL_H
#define _PLATFORM_INTERNAL_H

// QNX headers required for wamr to compile
#include <sys/platform.h>      // QNX -MUST COME FIRST!!
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h> 
#include <string.h>            
#include <stdlib.h>            
#include <unistd.h>
#include <inttypes.h>          // PRIx64
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Sanity check
#ifndef BH_PLATFORM_QNX7
#error "QNX7 not defined, build setup is corrupted".
#error "shared_plaform.cmake has not been included".
#endif

#ifndef bool
#define bool uint8_t
#endif

// As we don't currently support threads with QNX WASM we don't really need pthread, but (optional 
// and unused) API using it is defined in platform_api_vmcore.h so these need to be defined:
typedef pthread_t korp_tid;
typedef pthread_mutex_t korp_mutex;
typedef pthread_cond_t korp_cond;
typedef pthread_t korp_thread;
#define BH_THREAD_DEFAULT_PRIORITY 0

#ifdef __cplusplus
}
#endif

#endif /* end of _PLATFORM_INTERNAL_H */

