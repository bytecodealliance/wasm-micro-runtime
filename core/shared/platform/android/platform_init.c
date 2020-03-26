/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"
#include "platform_api_extension.h"
int
bh_platform_init()
{
    return 0;
}

void
bh_platform_destroy()
{
}

int os_printf(const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = __android_log_vprint(ANDROID_LOG_INFO, "wasm_runtime::", fmt, ap);
    va_end(ap);

    return ret;
}

int os_vprintf(const char *fmt, va_list ap)
{
    return __android_log_vprint(ANDROID_LOG_INFO, "wasm_runtime::", fmt, ap);
}

