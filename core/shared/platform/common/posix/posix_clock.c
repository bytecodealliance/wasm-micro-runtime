/*
 * Copyright (C) 2023 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"
#include "bh_platform.h"

#include <pthread.h>
#include <time.h>

bool
convert_clockid(bh_clock_id_t in, clockid_t *out)
{
    switch (in) {
        case BH_CLOCK_ID_MONOTONIC:
            *out = CLOCK_MONOTONIC;
            return true;
#if defined(CLOCK_PROCESS_CPUTIME_ID)
        case BH_CLOCK_ID_PROCESS_CPUTIME_ID:
            *out = CLOCK_PROCESS_CPUTIME_ID;
            return true;
#endif
        case BH_CLOCK_ID_REALTIME:
            *out = CLOCK_REALTIME;
            return true;
#if defined(CLOCK_THREAD_CPUTIME_ID)
        case BH_CLOCK_ID_THREAD_CPUTIME_ID:
            *out = CLOCK_THREAD_CPUTIME_ID;
            return true;
#endif
        default:
            errno = EINVAL;
            return false;
    }
}

int
os_clock_res_get(bh_clock_id_t clock_id, uint64 *resolution)
{
    clockid_t nclock_id;
    if (!convert_clockid(clock_id, &nclock_id))
        return BHT_ERROR;
    struct timespec ts;
    if (clock_getres(clock_id, &ts) < 0)
        return BHT_ERROR;
    *resolution = convert_timespec(&ts);

    return BHT_OK;
}

int
os_clock_time_get(bh_clock_id_t clock_id, uint64 precision, uint64 *time)
{
    clockid_t nclock_id;
    if (!convert_clockid(clock_id, &nclock_id))
        return BHT_ERROR;
    struct timespec ts;
    if (clock_gettime(nclock_id, &ts) < 0)
        return BHT_ERROR;
    *time = convert_timespec(&ts);

    return 0;
}

int
os_clock_sleep(bh_clock_id_t bh_clock_id, const struct timespec *timestamp, bool is_absolute_time)
{
#if CONFIG_HAS_CLOCK_NANOSLEEP == 1
        clockid_t clock_id;
        if (!convert_clockid(bh_clock_id, &clock_id))
            return BHT_ERROR;
        int ret = clock_nanosleep(
            clock_id,
            is_absolute_time
                ? TIMER_ABSTIME
                : 0,
            timestamp, NULL);
        return ret != 0 ? BHT_ERROR : BHT_OK;
#else
        switch (bh_clock_id) {
            case BH_CLOCK_ID_MONOTONIC:
                if (is_absolute_time) {
                    // TODO(ed): Implement.
                    fputs("Unimplemented absolute sleep on monotonic clock\n",
                          stderr);
                    errno = ENOSYS;
                    return BHT_ERROR;
                }
                else {
                    // Perform relative sleeps on the monotonic clock also using
                    // nanosleep(). This is incorrect, but good enough for now.
                    nanosleep(timestamp, NULL);
                }
                return BHT_OK;
            case BH_CLOCK_ID_REALTIME:
                if (is_absolute_time) {
                    // Sleeping to an absolute point in time can only be done
                    // by waiting on a condition variable.
                    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
                    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

                    pthread_mutex_lock(&mutex);
                    pthread_cond_timedwait(&cond, &mutex, timestamp);
                    pthread_mutex_unlock(&mutex);
                }
                else {
                    // Relative sleeps can be done using nanosleep().
                    nanosleep(timestamp, NULL);
                }
                return BHT_OK;
            default:
                errno = ENOTSUP;
                return BHT_ERROR;
        }
#endif
}