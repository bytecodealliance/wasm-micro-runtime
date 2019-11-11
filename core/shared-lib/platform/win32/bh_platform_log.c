/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include "bh_platform.h"

void bh_log_emit(const char *fmt, va_list ap)
{
    vprintf(fmt, ap);
    fflush(stdout);
}

int bh_fprintf(FILE *stream, const char *fmt, ...)
{
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = vfprintf(stream ? stream : stdout, fmt, ap);
    va_end(ap);
    return ret;
}

int bh_fflush(void *stream)
{
    return fflush(stream ? stream : stdout);
}
