/*
 * Copyright (C) 2023 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"
#include <winternl.h>

#define NANOSECONDS_PER_SECOND 1000000000ULL
#define NANOSECONDS_PER_TICK 100

static int
calculate_monotonic_clock_frequency(uint64 *out_frequency)
{
    LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency)) {
        return BHT_ERROR;
    }
    else {
        *out_frequency = (uint64)frequency.QuadPart;
        return BHT_OK;
    }
}

// The implementation below derives from the following source:
// https://github.com/WasmEdge/WasmEdge/blob/b70f48c42922ce5ee7730054b6ac0b1615176285/lib/host/wasi/win.h#L210
static uint64
filetime_to_wasi_timestamp(FILETIME filetime)
{
    static const uint64 ntto_unix_epoch =
        134774ULL * 86400ULL * NANOSECONDS_PER_SECOND;

    ULARGE_INTEGER temp = { .LowPart = filetime.dwLowDateTime,
                            .HighPart = filetime.dwHighDateTime };

    return (temp.QuadPart * 100ull) - ntto_unix_epoch;
}

static int
get_performance_counter_value(uint64 *out_counter)
{
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter)) {
        return BHT_ERROR;
    }
    else {
        *out_counter = counter.QuadPart;
        return BHT_OK;
    }
}

int
os_clock_res_get(bh_clock_id_t clock_id, uint64 *resolution)
{
    switch (clock_id) {
        case BH_CLOCK_ID_MONOTONIC:
        {
            uint64 frequency;
            if (calculate_monotonic_clock_frequency(&frequency) == BHT_ERROR) {
                return BHT_ERROR;
            }
            const uint64 result = (uint64)NANOSECONDS_PER_SECOND / frequency;
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
            FILETIME sys_now;
#if NTDDI_VERSION >= NTDDI_WIN8
            GetSystemTimePreciseAsFileTime(&sys_now);
#else
            GetSystemTimeAsFileTime(&SysNow);
#endif
            *time = filetime_to_wasi_timestamp(sys_now);
            return BHT_OK;
        }
        case BH_CLOCK_ID_MONOTONIC:
        {
            uint64 frequency;
            if (calculate_monotonic_clock_frequency(&frequency) == BHT_ERROR) {
                return BHT_ERROR;
            }
            uint64 counter;
            if (get_performance_counter_value(&counter) == BHT_ERROR) {
                return BHT_ERROR;
            }
            if (NANOSECONDS_PER_SECOND % frequency == 0) {
                *time = counter * NANOSECONDS_PER_SECOND / frequency;
            }
            else {
                uint64 seconds = counter / frequency;
                uint64 fractions = counter % frequency;
                *time = seconds * NANOSECONDS_PER_SECOND
                        + (fractions * NANOSECONDS_PER_SECOND) / frequency;
            }
            return BHT_OK;
        }
        case BH_CLOCK_ID_PROCESS_CPUTIME_ID:
        {
            FILETIME creation_time;
            FILETIME exit_time;
            FILETIME kernel_time;
            FILETIME user_time;

            if (!GetProcessTimes(GetCurrentProcess(), &creation_time,
                                 &exit_time, &kernel_time, &user_time)) {
                return BHT_ERROR;
            }
            *time = filetime_to_wasi_timestamp(kernel_time)
                    + filetime_to_wasi_timestamp(user_time);

            return BHT_OK;
        }
        case BH_CLOCK_ID_THREAD_CPUTIME_ID:
        {
            FILETIME creation_time;
            FILETIME exit_time;
            FILETIME kernel_time;
            FILETIME user_time;

            if (!GetProcessTimes(GetCurrentThread(), &creation_time, &exit_time,
                                 &kernel_time, &user_time)) {
                return BHT_ERROR;
            }

            *time = filetime_to_wasi_timestamp(kernel_time)
                    + filetime_to_wasi_timestamp(user_time);

            return BHT_OK;
        }
        default:
            errno = EINVAL;
            return BHT_ERROR;
    }
}