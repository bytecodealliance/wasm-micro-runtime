/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"

int
bh_platform_init()
{
    return 0;
}

void
bh_platform_destroy()
{}

int
os_vprintf(const char *format, va_list ap)
{
#ifndef BH_VPRINTF
    return ptr_vprintf(format, ap);
#else
    return BH_VPRINTF(format, ap);
#endif
}

int
os_printf(const char *format, ...)
{
    int ret;
    va_list ap;

    va_start(ap, format);
    ret = os_vprintf(format, ap);
    va_end(ap);

    return ret;
}
