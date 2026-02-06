/*
 * Copyright (C) 2023 Midokura Japan KK.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <time.h>

#include "platform_api_extension.h"

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

/*
 * This function relies on a small workaround (hardcoded errno values)
 * to avoid including <errno.h> or the project-specific `libc_errno.h`,
 * thus preventing changes to the current CMake configuration.
 */
__wasi_errno_t
os_nanosleep(const os_timespec *req, os_timespec *rem)
{
    int ret = 0;

    ret = nanosleep(req, rem);

    switch(ret)
    {
        case 14 /* EFAULT */: return __WASI_EFAULT;
        case  4 /* EINTR  */: return __WASI_EFAULT;
        case 22 /* EINVAL */: return __WASI_EINVAL;
        case  0: return __WASI_ESUCCESS;
    }
}