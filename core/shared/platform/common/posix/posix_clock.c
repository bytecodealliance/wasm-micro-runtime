/*
 * Copyright (C) 2023 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"

#define NANOSECONDS_PER_SECOND 1000000000ULL

static bool
bh_clockid_to_clockid(bh_clock_id_t in, clockid_t *out)
{
    switch (in) {
        case BH_CLOCK_ID_MONOTONIC:
            *out = CLOCK_MONOTONIC;
            return true;
#if defined(CLOCK_PROCESS_CPUTIME_ID)
        case BH_CLOCK_ID_PROCESS_CPUTIME_ID:
            *out = CLOCK_PROCESS_CPUTIME_ID;
            return true;
#endif
        case BH_CLOCK_ID_REALTIME:
            *out = CLOCK_REALTIME;
            return true;
#if defined(CLOCK_THREAD_CPUTIME_ID)
        case BH_CLOCK_ID_THREAD_CPUTIME_ID:
            *out = CLOCK_THREAD_CPUTIME_ID;
            return true;
#endif
        default:
            errno = EINVAL;
            return false;
    }
}

static uint64
timespec_to_nanoseconds(const struct timespec *ts)
{
    if (ts->tv_sec < 0)
        return 0;
    if ((uint64)ts->tv_sec >= UINT64_MAX / NANOSECONDS_PER_SECOND)
        return UINT64_MAX;
    return (uint64)ts->tv_sec * NANOSECONDS_PER_SECOND + (uint64)ts->tv_nsec;
}

int
os_clock_res_get(bh_clock_id_t clock_id, uint64 *resolution)
{
    clockid_t nclock_id;
    if (!bh_clockid_to_clockid(clock_id, &nclock_id))
        return BHT_ERROR;
    struct timespec ts;
    if (clock_getres(nclock_id, &ts) < 0)
        return BHT_ERROR;
    *resolution = timespec_to_nanoseconds(&ts);

    return BHT_OK;
}

int
os_clock_time_get(bh_clock_id_t clock_id, uint64 precision, uint64 *time)
{
    clockid_t nclock_id;
    if (!bh_clockid_to_clockid(clock_id, &nclock_id))
        return BHT_ERROR;
    struct timespec ts;
    if (clock_gettime(nclock_id, &ts) < 0)
        return BHT_ERROR;
    *time = timespec_to_nanoseconds(&ts);

    return 0;
}
