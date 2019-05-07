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

#ifndef NVALGRIND
#define NVALGRIND
#endif

/* Reserve bytes on applet stack for native functions called from
 * Java methods to avoid undetectable stack overflow.
 */
#ifndef APPLET_PRESERVED_STACK_SIZE
#define APPLET_PRESERVED_STACK_SIZE (16 * BH_KB)
#endif

typedef unsigned __int64 uint64;
typedef __int64 int64;

extern void DEBUGME(void);

#define DIE do{bh_debug("Die here\n\n\n\n\n\n\n\n\n\n\n\n\n\n"); DEBUGME(void); while(1);}while(0)

#ifndef BH_INVALID_HANDLE
#define BH_INVALID_HANDLE NULL
#endif

#define BH_PLATFORM "AMULET"

#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#define _STACK_SIZE_ADJUSTMENT (32 * 1024)

/* Stack size of applet manager thread.  */
#define BH_APPLET_MANAGER_THREAD_STACK_SIZE (8 * 1024 + _STACK_SIZE_ADJUSTMENT)

/* Stack size of HMC thread.  */
#define BH_HMC_THREAD_STACK_SIZE            (4 * 1024 + _STACK_SIZE_ADJUSTMENT)

/* Stack size of watchdog thread.  */
#define BH_WATCHDOG_THREAD_SIZE             (4 * 1024 + _STACK_SIZE_ADJUSTMENT)

/* Stack size of applet threads's native part.  */
#define BH_APPLET_PRESERVED_STACK_SIZE      (8 * 1024 + _STACK_SIZE_ADJUSTMENT)

/* Maximal recursion depth of interpreter.  */
#define BH_MAX_INTERP_RECURSION_DEPTH       8

#define wa_malloc bh_malloc
#define wa_free bh_free

#define snprintf _snprintf

#define BH_ROUTINE_MODIFIER __stdcall

typedef void *korp_tid;
#define INVALID_THREAD_ID 0

typedef void *korp_mutex;
typedef void *korp_sem;

typedef struct {
    korp_sem s;
    unsigned waiting_count;
} korp_cond;

typedef void* (BH_ROUTINE_MODIFIER *thread_start_routine_t)(void*);

#endif

