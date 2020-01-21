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
  return aos_get_hz() / 1000;
}

/*
 * This function returns milliseconds after boot.
 * @return milliseconds after boot.
 */
uint64 _bh_time_get_boot_millisecond()
{
  return (uint64)aos_now_ms();
}

uint64 _bh_time_get_millisecond_from_1970()
{
  return (uint64)aos_now_ms();
}

size_t
_bh_time_strftime (char *str, size_t max, const char *format, int64 time)
{
  str = aos_now_time_str(str, max);
  return str ? strlen(str) + 1 : 0;
}

