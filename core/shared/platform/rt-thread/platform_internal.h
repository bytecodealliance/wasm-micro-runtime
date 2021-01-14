/*
 * Copyright (c) 2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef RTTHREAD_PLATFORM_INTERNAL_H
#define RTTHREAD_PLATFORM_INTERNAL_H

#include <rtthread.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <ctype.h>

typedef rt_thread_t korp_tid;
typedef struct rt_mutex korp_mutex;
typedef struct rt_thread korp_cond;
typedef struct rt_thread korp_thread;

typedef rt_uint8_t uint8_t;
typedef rt_int8_t int8_t;
typedef rt_uint16_t uint16_t;
typedef rt_int16_t int16_t;
typedef rt_uint64_t uint64_t;
typedef rt_int64_t int64_t;


#endif //RTTHREAD_PLATFORM_INTERNAL_H
