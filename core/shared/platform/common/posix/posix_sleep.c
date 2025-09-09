/*
 * Copyright (C) 2023 Midokura Japan KK.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <time.h>

#include "platform_api_extension.h"
#include "libc_errno.h"

int
os_usleep(uint32 usec)
{
    struct timespec ts;
    int ret;

    ts.tv_sec = usec / 1000000;
    ts.tv_nsec = (usec % 1000000) * 1000;
    ret = nanosleep(&ts, NULL);
    return ret == 0 ? 0 : -1;
}

__wasi_errno_t
os_nanosleep(os_timespec *req, os_timespec *rem)
{
    int ret;

    ret = nanosleep(req, rem);

    return convert_errno(ret);
}