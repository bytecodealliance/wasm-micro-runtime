/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include <stdio.h>

#include <android/log.h>

void bh_log_emit(const char *fmt, va_list ap)
{
    (void)__android_log_vprint(ANDROID_LOG_INFO, "wasm_runtime::", fmt, ap);
}

int bh_fprintf(FILE *stream, const char *fmt, ...)
{
    (void)stream;
    va_list ap;
    int ret = 0;

    va_start(ap, fmt);
    ret = __android_log_vprint(ANDROID_LOG_INFO, "wasm_runtime::", fmt, ap);
    va_end(ap);

    return ret;
}

int bh_fflush(void *stream)
{
    (void)stream;
    return __android_log_print(ANDROID_LOG_INFO, "wasm_runtime::", "%s", "NOT IMPLEMENT");
}
