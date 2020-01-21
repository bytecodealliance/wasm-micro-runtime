/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_time.h"

/*
 * This function returns milliseconds per tick.
 * @return milliseconds per tick.
 */
uint64 _bh_time_get_tick_millisecond()
{
    return k_uptime_get_32();
}

/*
 * This function returns milliseconds after boot.
 * @return milliseconds after boot.
 */
uint64 _bh_time_get_boot_millisecond()
{
    return k_uptime_get_32();
}

uint64 _bh_time_get_millisecond_from_1970()
{
    return k_uptime_get();
}

size_t _bh_time_strftime(char *str, size_t max, const char *format, int64 time)
{
    (void) format;
    (void) time;
    uint32 t = k_uptime_get_32();
    int h, m, s;

    t = t % (24 * 60 * 60);
    h = t / (60 * 60);
    t = t % (60 * 60);
    m = t / 60;
    s = t % 60;

    return snprintf(str, max, "%02d:%02d:%02d", h, m, s);
}

