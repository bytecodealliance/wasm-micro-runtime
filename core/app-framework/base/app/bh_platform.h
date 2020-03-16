/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef DEPS_IWASM_APP_LIBS_BASE_BH_PLATFORM_H_
#define DEPS_IWASM_APP_LIBS_BASE_BH_PLATFORM_H_

#include <stdbool.h>

typedef unsigned char uint8;
typedef char int8;
typedef unsigned short uint16;
typedef short int16;
typedef unsigned int uint32;
typedef int int32;

#ifndef NULL
#  define NULL ((void*) 0)
#endif

#ifndef __cplusplus
#define true 1
#define false 0
#define inline __inline
#endif

// all wasm-app<->native shared source files should use WA_MALLOC/WA_FREE.
// they will be mapped to different implementations in each side
#ifndef WA_MALLOC
#define WA_MALLOC malloc
#endif

#ifndef WA_FREE
#define WA_FREE free
#endif

char *wa_strdup(const char *s);

uint32 htonl(uint32 value);
uint32 ntohl(uint32 value);
uint16 htons(uint16 value);
uint16 ntohs(uint16 value);

int
b_memcpy_s(void * s1, unsigned int s1max, const void * s2, unsigned int n);

#endif /* DEPS_IWASM_APP_LIBS_BASE_BH_PLATFORM_H_ */
