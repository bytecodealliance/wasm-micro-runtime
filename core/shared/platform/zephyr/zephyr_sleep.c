/*
 * Copyright (C) 2024 Grenoble INP - ESISAR.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include "platform_api_extension.h"

/*
 * In Zephyr v3.7, there is no simple way to get a `nanosleep` implementation.
 * But in the later version the Zephyr community introduced some clock APIs
 * and their POSIX compatibility layer. 
 *
 * Relevant Zephyr sources:
 *     - zephyr/include/zephyr/sys/clock.h 
 *     - Zephyr/lib/os/clock.c
 * POSIX layer:
 *     - zephyr/lib/posix/options/clock.c
 *
 * Instead of re-implementing the full Clock APIs, this file provides a naive
 * `nanosleep` implementation based on the Zephyr thread API (`k_sleep`).
 * 
 * Limitations:
 *     Maximum sleep duration is limited by UINT32_MAX or UINT64_MAX ticks
 *     (≈ 4,294,967,295 and 18,446,744,073,709,551,615 respectively).
 *
 * Example at a "slow" clock rate of 50 kHz:
 *     - UINT32_MAX: ~85 899s (~23 hours)
 *     - UINT64_MAX: ~368 934 881 474 191s (~11.7 millions years)
 * Clearly, `nanosleep` should not be used for such long durations.
 *
 * Note: this assumes `CONFIG_POSIX_API=n` in the Zephyr application.
 */

#ifdef CONFIG_TIMEOUT_64BIT
static int64_t timespec_to_ticks(const os_timespec *ts);
#else
static uint32_t timespec_to_ticks(const os_timespec *ts);
#endif

__wasi_errno_t
os_nanosleep(const os_timespec *req, os_timespec *rem)
{
    k_timeout_t timeout;

    if (req == NULL){
        return __WASI_EINVAL;
    }

    timeout.ticks = (k_ticks_t) timespec_to_ticks(req);

    /*
     * The function `int32_t k_sleep(k_timeout_t timeout)` return either:
     *     * 0     requested time elaspsed.
     *     * >0    remaining time in ms (due to k_wakeup).
     */
    int32_t rc = k_sleep(timeout);
    if (rem != NULL && 0 < rc){
        rem->tv_sec  = rc / 1000;
        rem->tv_nsec = ( rc % 1000 ) * 1000000;
    }

    return __WASI_ESUCCESS;
}


#ifdef CONFIG_TIMEOUT_64BIT

static int64_t timespec_to_ticks(const os_timespec *ts)
{
    const uint64_t ticks_per_sec = CONFIG_SYS_CLOCK_TICKS_PER_SEC;
    uint64_t ticks = 0;

    if (ts->tv_sec > UINT64_MAX / ticks_per_sec) {
        return UINT64_MAX;
    }

    ticks = (uint64_t)ts->tv_sec * ticks_per_sec;

    if (ts->tv_nsec > 0) {
        uint64_t add = (uint64_t)ts->tv_nsec * ticks_per_sec / 1000000000ULL;
        if (ticks > UINT64_MAX - add) {
            return UINT64_MAX; 
        }
        ticks += add;
    }

    return ticks;
}

#else /* CONFIG_TIMEOUT_32BIT */

static uint32_t timespec_to_ticks(const os_timespec *ts)
{
    const uint32_t ticks_per_sec = CONFIG_SYS_CLOCK_TICKS_PER_SEC;
    uint32_t ticks = 0;

    if (ts->tv_sec > UINT32_MAX / ticks_per_sec) {
        return UINT32_MAX;
    }

    ticks = (uint32_t)ts->tv_sec * ticks_per_sec;

    if (ts->tv_nsec > 0) {
        uint64_t add64 = (uint64_t)ts->tv_nsec * ticks_per_sec;
        uint32_t add = (uint32_t)(add64 / 1000000000ULL);
        if (ticks > UINT32_MAX - add) {
            return UINT32_MAX; 
        }
        ticks += add;
    }

    return ticks;
}

#endif /* CONFIG_TIMEOUT_64BIT */