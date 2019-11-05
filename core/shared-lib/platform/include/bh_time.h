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

#ifndef _BH_TIME_H
#define _BH_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bh_config.h"
#include "bh_types.h"
#include "bh_platform.h"

/*
 * This function returns milliseconds per tick.
 * @return milliseconds per tick.
 */
extern uint64 _bh_time_get_tick_millisecond(void);
#ifdef _INSTRUMENT_TEST_ENABLED
extern uint64 bh_time_get_tick_millisecond_instr(const char*func_name);
#define bh_time_get_tick_millisecond() bh_time_get_tick_millisecond_instr(__FUNCTION__)
#else
#define bh_time_get_tick_millisecond _bh_time_get_tick_millisecond
#endif

/*
 * This function returns milliseconds after boot.
 * @return milliseconds after boot.
 */
extern uint64 _bh_time_get_boot_millisecond(void);
#ifdef _INSTRUMENT_TEST_ENABLED
extern uint64 bh_time_get_boot_millisecond_instr(const char*func_name);
#define bh_time_get_boot_millisecond() bh_time_get_boot_millisecond_instr(__FUNCTION__)
#else
#define bh_time_get_boot_millisecond _bh_time_get_boot_millisecond
#endif

extern uint32 bh_get_tick_sec();
#define  bh_get_tick_ms _bh_time_get_boot_millisecond

/*
 * This function returns GMT milliseconds since from 1970.1.1, AKA UNIX time.
 * @return milliseconds since from 1970.1.1.
 */
extern uint64 _bh_time_get_millisecond_from_1970(void);
#ifdef _INSTRUMENT_TEST_ENABLED
extern uint64 bh_time_get_millisecond_from_1970_instr(const char*func_name);
#define bh_time_get_millisecond_from_1970() bh_time_get_millisecond_from_1970_instr(__FUNCTION__)
#else
#define bh_time_get_millisecond_from_1970 _bh_time_get_millisecond_from_1970
#endif

/**
 * This function sets timezone with specific hours.
 *
 * @param hours  represents the deviation (in hours) of the local time from GMT (can be a positive or a negative number)
 * @param half_hour  if true, adds half an hour to the local time calculation. For example, if hours=(+5) then the time will be GMT +5:30; if hours=(-5) then the time will be GMT -4:30.
 * @param daylight_save  if true, applies the daylight saving scheme when calculating the local time (adds one hour to the local time calculation)
 */
extern void bh_set_timezone(int hours, int half_hour, int daylight_save);

/**
 * This functions returns the offset in seconds which needs to be added GMT to get the local time.
 *
 *
 * @return offset in secords which needs to be added GMT to get the local time.
 */
extern int bh_get_timezone_offset(void);

size_t bh_time_strftime(char *s, size_t max, const char *format, int64 time);

#ifdef _INSTRUMENT_TEST_ENABLED
size_t bh_time_strftime_instr(char *s, size_t max, const char *format, int64 time, const char*func_name);
#define bh_time_strftime(s, max, format, time) bh_time_strftime_instr(s, max, format, time, __FUNCTION__)
#else
#define bh_time_strftime _bh_time_strftime
#endif

#ifdef __cplusplus
}
#endif

#endif
