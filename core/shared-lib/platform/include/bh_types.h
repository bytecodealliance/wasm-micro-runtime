/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _BH_TYPES_H
#define _BH_TYPES_H

#include "bh_config.h"

typedef unsigned char uint8;
typedef char int8;
typedef unsigned short uint16;
typedef short int16;
typedef unsigned int uint32;
typedef int int32;
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

