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

