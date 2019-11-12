/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_time.h"

#include <unistd.h>
#include <stdio.h>
//#include <sys/timeb.h>
#include <time.h>

/*
 * This function returns milliseconds per tick.
 * @return milliseconds per tick.
 */
uint64 _bh_time_get_tick_millisecond()
{
    return sysconf(_SC_CLK_TCK);
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

    return ((uint64) ts.tv_sec) * 1000 + ts.tv_nsec / (1000 * 1000);
}

/*
 * This function returns GMT time milliseconds since from 1970.1.1, AKA UNIX time.
 * @return milliseconds since from 1970.1.1.
 */
uint64 _bh_time_get_millisecond_from_1970()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64 millisecondsSinceEpoch = (uint64_t)(tv.tv_sec) * 1000
            + (uint64_t)(tv.tv_usec) / 1000;
    return millisecondsSinceEpoch;
}

size_t _bh_time_strftime(char *s, size_t max, const char *format, int64 time)
{
    time_t time_sec = time / 1000;
    struct tm *ltp;

    ltp = localtime(&time_sec);
    if (ltp == NULL) {
        return 0;
    }
    return strftime(s, max, format, ltp);
}

