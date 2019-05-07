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

// all wasm-app<->native shared source files should use wa_malloc/wa_free.
// they will be mapped to different implementations in each side
#ifndef wa_malloc
#define wa_malloc malloc
#endif

#ifndef wa_free
#define wa_free free
#endif

char *wa_strdup(const char *s);

uint32 htonl(uint32 value);
uint32 ntohl(uint32 value);
uint16 htons(uint16 value);
uint16 ntohs(uint16 value);

int
b_memcpy_s(void * s1, unsigned int s1max, const void * s2, unsigned int n);

#endif /* DEPS_IWASM_APP_LIBS_BASE_BH_PLATFORM_H_ */
