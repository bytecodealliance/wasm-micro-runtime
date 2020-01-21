/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
