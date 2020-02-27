/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_time.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>

/*
 * This function returns milliseconds per tick.
 * @return milliseconds per tick.
 */
uint64 _bh_time_get_tick_millisecond()
{
    return (uint64)sysconf(_SC_CLK_TCK);
}

/*
 * This function returns milliseconds after boot.
 * @return milliseconds after boot.
 */
uint64 _bh_time_get_boot_millisecond()
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return ((uint64) ts.tv_sec) * 1000 + ((uint64)ts.tv_nsec) / (1000 * 1000);
}

uint32 bh_get_tick_sec()
{
    return (uint32)(_bh_time_get_boot_millisecond() / 1000);
}

/*
 * This function returns GMT time milliseconds since from 1970.1.1, AKA UNIX time.
 * @return milliseconds since from 1970.1.1.
 */
uint64 _bh_time_get_millisecond_from_1970()
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);

   return tv.tv_sec * 1000 + tv.tv_usec
           - (tz.tz_dsttime == 0 ? 0 : 60 * 60 * 1000)
           + tz.tz_minuteswest * 60 * 1000;
}

size_t _bh_time_strftime(char *s, size_t max, const char *format, int64 time)
{
    time_t time_sec = (time_t)(time / 1000);
    struct timeval tv;
    struct timezone tz;
    struct tm *ltp;

    gettimeofday(&tv, &tz);
    time_sec -= tz.tz_minuteswest * 60;

    ltp = localtime(&time_sec);
    if (ltp == NULL) {
        return 0;
    }
    return strftime(s, max, format, ltp);
}

