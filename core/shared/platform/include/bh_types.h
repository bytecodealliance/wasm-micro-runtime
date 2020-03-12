/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _BH_TYPES_H
#define _BH_TYPES_H

#include "bh_config.h"
#include <stdint.h>

typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef float float32;
typedef double float64;

#include "bh_platform.h"

#ifndef NULL
#define NULL (void*)0
#endif

#ifndef __cplusplus
#define true 1
#define false 0
#define inline __inline
#endif

#endif

