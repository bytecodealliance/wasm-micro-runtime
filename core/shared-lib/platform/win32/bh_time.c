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

#include <stdio.h>
#include "bh_time.h"

#include <time.h>
#include <sys/timeb.h>

/* Since GetTickCount64 is not supported on Windows XP, use a temporary method
 * to solve the issue. However, GetTickCount return a DWORD value and overflow
 * may happen (http://msdn.microsoft.com/en-us/library/ms724408(v=vs.85).aspx).
 *
 * TODO: Implement GetTickCount64 on Windows XP by self or check overflow issues.
 */
#if (WINVER >= 0x0600)
extern uint64 __stdcall GetTickCount64(void);
#else
extern uint32 __stdcall GetTickCount(void);
#endif

/*
 * This function returns milliseconds per tick.
 * @return milliseconds per tick.
 */
uint64 _bh_time_get_tick_millisecond()
{
    return 5;
}

/*
 * This function returns milliseconds after boot.
 * @return milliseconds after boot.
 */
uint64 _bh_time_get_boot_millisecond()
{
    /* Since GetTickCount64 is not supported on Windows XP, use a temporary method
     * to solve the issue. However, GetTickCount return a DWORD value and overflow
     * may happen (http://msdn.microsoft.com/en-us/library/ms724408(v=vs.85).aspx).
     *
     * TODO: Implement GetTickCount64 on Windows XP by self or check overflow issues.
     */
#if (WINVER >= 0x0600)
    return GetTickCount64();
#else
    return GetTickCount();
#endif
}

/*
 * This function returns GMT time milliseconds since from 1970.1.1, AKA UNIX time.
 * @return milliseconds since from 1970.1.1.
 */
uint64 _bh_time_get_millisecond_from_1970()
{
    struct timeb tp;
    ftime(&tp);

    return ((uint64) tp.time) * 1000 + tp.millitm
            - (tp.dstflag == 0 ? 0 : 60 * 60 * 1000) + tp.timezone * 60 * 1000;
}

size_t bh_time_strftime(char *s, size_t max, const char *format, int64 time)
{
    time_t time_sec = time / 1000;
    struct timeb tp;
    struct tm local_time;

    if (NULL == s)
        return 0;

    ftime(&tp);
    time_sec -= tp.timezone * 60;
    if (localtime_s(&local_time, &time_sec) != 0)
        return 0;

    return strftime(s, max, format, &local_time);
}

int bh_time_get(uint8 *timeoff_info, int16 timeoff_info_len, uint32* time)
{
    return BH_UNIMPLEMENTED;
}

int bh_time_set(uint32 ntp, uint8 *timeoff_info)
{
    return BH_UNIMPLEMENTED;
}
