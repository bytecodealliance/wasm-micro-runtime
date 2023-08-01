/*
 * Copyright (C) 2023 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_time.h"

uint64
convert_timespec(const struct timespec *ts)
{
    if (ts->tv_sec < 0)
        return 0;
    if ((uint64)ts->tv_sec >= UINT64_MAX / 1000000000)
        return UINT64_MAX;
    return (uint64)ts->tv_sec * 1000000000 + (uint64)ts->tv_nsec;
}
