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

#ifndef BH_PLATFORM_LOG
#define BH_PLATFORM_LOG

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

void bh_log_emit(const char *fmt, va_list ap);

int bh_fprintf(void *stream, const char *fmt, ...);

int bh_fflush(void *stream);

#ifdef __cplusplus
}
#endif

#endif
