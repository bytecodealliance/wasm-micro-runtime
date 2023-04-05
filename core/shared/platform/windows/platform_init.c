/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"

int
os_thread_sys_init();

void
os_thread_sys_destroy();

int
init_winsock();

void
deinit_winsock();

int
bh_platform_init()
{
    if (init_winsock() != 0) {
        return -1;
    }

    return os_thread_sys_init();
}

void
bh_platform_destroy()
{
    deinit_winsock();

    os_thread_sys_destroy();
}

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

unsigned
os_getpagesize()
{
    SYSTEM_INFO sys_info;
    GetNativeSystemInfo(&sys_info);
    return (unsigned)sys_info.dwPageSize;
}

void
os_dcache_flush(void)
{}
