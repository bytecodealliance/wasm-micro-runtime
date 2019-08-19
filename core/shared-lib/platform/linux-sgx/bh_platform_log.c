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

#include "bh_platform.h"
#include <stdio.h>

void bh_log_emit(const char *fmt, va_list ap)
{
    vprintf(fmt, ap);
    fflush(stdout);
}

int bh_fprintf(FILE *stream, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vfprintf(stream ? stream : stdout, fmt, ap);
    va_end(ap);

    return ret;
}

int bh_fflush(void *stream)
{
    return fflush(stream ? stream : stdout);
}
