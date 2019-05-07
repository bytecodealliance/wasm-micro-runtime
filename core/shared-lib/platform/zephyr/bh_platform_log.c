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

struct out_context {
    int count;
};

typedef int (*out_func_t)(int c, void *ctx);

extern void *__printk_get_hook(void);
static int (*_char_out)(int) = NULL;

static int char_out(int c, struct out_context *ctx)
{
    ctx->count++;
    if (_char_out == NULL) {
        _char_out = __printk_get_hook();
    }
    return _char_out(c);
}

static int bh_vprintk(const char *fmt, va_list ap)
{
    struct out_context ctx = { 0 };
    _vprintk((out_func_t) char_out, &ctx, fmt, ap);
    return ctx.count;
}

void bh_log_emit(const char *fmt, va_list ap)
{
    bh_vprintk(fmt, ap);
}

int bh_fprintf(FILE *stream, const char *fmt, ...)
{
    (void) stream;
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = bh_vprintk(fmt, ap);
    va_end(ap);

    return ret;
}

int bh_fflush(void *stream)
{
    (void) stream;
    return 0;
}

