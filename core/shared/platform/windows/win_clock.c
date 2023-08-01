/*
 * Copyright (C) 2023 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"
#include <winternl.h>

#define NANOSECONDS_PER_SECOND 1000000000
#define NANOSECONDS_PER_TICK 100

static uint64
calculate_monotonic_clock_frequency()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return (uint64)frequency.QuadPart;
}

// The implementation below dervies from the following source:
// https://github.com/WasmEdge/WasmEdge/blob/b70f48c42922ce5ee7730054b6ac0b1615176285/lib/host/wasi/win.h#L210
static uint64
from_file_time_to_wasi_timestamp(FILETIME filetime)
{
    static const uint64 ntto_unix_epoch =
        134774 * 86400 * NANOSECONDS_PER_SECOND;

    ULARGE_INTEGER temp = { .LowPart = filetime.dwLowDateTime,
                            .HighPart = filetime.dwHighDateTime };

    return temp.QuadPart - ntto_unix_epoch;
}

uint64
current_value_of_peformance_counter()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
}

int
os_clock_res_get(bh_clock_id_t clock_id, uint64 *resolution)
{
    switch (clock_id) {
        case BH_CLOCK_ID_MONOTONIC:
        {
            const uint64 result = (uint64)NANOSECONDS_PER_SECOND
                                  / calculate_monotonic_clock_frequency();
            *resolution = result;
            return BHT_OK;
        }
        case BH_CLOCK_ID_REALTIME:
        case BH_CLOCK_ID_PROCESS_CPUTIME_ID:
        case BH_CLOCK_ID_THREAD_CPUTIME_ID:
        {
            PULONG maximum_time;
            PULONG minimum_time;
            PULONG current_time;
            NTSTATUS
            status = NtQueryTimerResolution(&maximum_time, &minimum_time,
                                            &current_time);

            uint64 result = (uint64)current_time * NANOSECONDS_PER_TICK;
            *resolution = result / (uint64)NANOSECONDS_PER_SECOND;
            return BHT_OK;
        }
        default:
            errno = EINVAL;
            return BHT_ERROR;
    }
}

int
os_clock_time_get(bh_clock_id_t clock_id, uint64 precision, uint64 *time)
{
    switch (clock_id) {
        case BH_CLOCK_ID_REALTIME:
        {
            FILETIME sysNow;
#if NTDDI_VERSION >= NTDDI_WIN8
            GetSystemTimePreciseAsFileTime(&sysNow);
#else
            GetSystemTimeAsFileTime(&SysNow);

#endif
            *time = from_file_time_to_wasi_timestamp(sysNow);
            return BHT_OK;
        }

        case BH_CLOCK_ID_MONOTONIC:
        {
            uint64 counter = current_value_of_peformance_counter();
            uint64 frequency = calculate_monotonic_clock_frequency();
            if (NANOSECONDS_PER_SECOND % frequency == 0) {
                *time = counter * NANOSECONDS_PER_SECOND / frequency;
            }
            else {
                uint64 seconds = counter / frequency;
                uint64 fractions = counter % frequency;
                *time = seconds * NANOSECONDS_PER_SECOND
                        + (fractions * NANOSECONDS_PER_SECOND) / frequency;
            }
        }
            return BHT_OK;

        case BH_CLOCK_ID_PROCESS_CPUTIME_ID:
        {
            FILETIME creation_time;
            FILETIME exit_time;
            FILETIME kernal_time;
            FILETIME user_time;

            if (!GetProcessTimes(GetCurrentProcess(), &creation_time,
                                 &exit_time, &kernal_time, &user_time)) {

                return BHT_ERROR;
            }
            *time = from_file_time_to_wasi_timestamp(kernal_time)
                    + from_file_time_to_wasi_timestamp(user_time);

            return BHT_OK;
        }

        case BH_CLOCK_ID_THREAD_CPUTIME_ID:
        {
            FILETIME creation_time;
            FILETIME exit_time;
            FILETIME kernal_time;
            FILETIME user_time;

            if (!GetProcessTimes(GetCurrentThread(), &creation_time, &exit_time,
                                 &kernal_time, &user_time)) {

                return BHT_ERROR;
            }

            *time = from_file_time_to_wasi_timestamp(kernal_time)
                    + from_file_time_to_wasi_timestamp(user_time);

            return BHT_OK;
        }
        default:
            errno = EINVAL;
            return BHT_ERROR;
    }
}