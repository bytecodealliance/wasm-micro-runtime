/*
 * Copyright (C) 2024 Grenoble INP - ESISAR.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_extension.h"

// #include <zephyr/kernel.h>   

/*
 * Assuming CONFIG_POSIX_API=n
 * Inspired zephyr/lib/posix/options/clock.c
 */

__wasi_errno_t
os_nanosleep(os_timespec *req, os_timespec *rem)
{
    // __wasi_errno_t ret;

    // if (req == NULL){
    //     return __WASI_EINVAL;
    // }

    // /*
    //  * os_timespec is typedef'ed to struct timespec so it's one to one.
    //  * Also sys_clock_nanosleep return either: 
    //  *     * 0       on sucess
    //  *     * -EINVAL on failure 
    //  */
    // int rc = sys_clock_nanosleep(SYS_CLOCK_REALTIME, 0, req, rem);
    // if (0 > rc){
    //     return __WASI_EINVAL;
    // }

    return __WASI_ESUCCESS;
}

/*
 * Don't exist in v3.7
 *
 * Inspired zephyr/lib/posix/options/clock.c on main  
 */

// int sys_clock_nanosleep(int clock_id, int flags, const struct timespec *rqtp,
// 			       struct timespec *rmtp)
// {
// 	k_timepoint_t end;
// 	k_timeout_t timeout;
// 	struct timespec duration;
// 	const bool update_rmtp = rmtp != NULL;
// 	const bool abstime = (flags & SYS_TIMER_ABSTIME) != 0;


//     /*
//      * Arguments checks
//      */
//     if((clock_id != SYS_CLOCK_MONOTONIC) &&
// 	   (clock_id != SYS_CLOCK_REALTIME)){
//         return __WASI_EINVAL;
//     }

//     if((rqtp->tv_sec < 0)  ||
//        (rqtp->tv_nsec < 0) ||
//        (rqtp->tv_nsec >= (long)NSEC_PER_SEC)){
//         return __WASI_EINVAL;
//     }
    

// 	if (abstime) {
// 		/* convert absolute time to relative time duration */
// 		(void)sys_clock_gettime(clock_id, &duration);
// 		(void)timespec_negate(&duration);
// 		(void)timespec_add(&duration, rqtp);
// 	} else {
// 		duration = *rqtp;
// 	}

// 	/* sleep for relative time duration */
// 	if ((sizeof(rqtp->tv_sec) == sizeof(int64_t)) &&
// 	    unlikely(rqtp->tv_sec >= (time_t)(UINT64_MAX / NSEC_PER_SEC))) {
// 		uint64_t ns = (uint64_t)k_sleep(K_SECONDS(duration.tv_sec - 1)) * NSEC_PER_MSEC;
// 		struct timespec rem = {
// 			.tv_sec = (time_t)(ns / NSEC_PER_SEC),
// 			.tv_nsec = ns % NSEC_PER_MSEC,
// 		};

// 		duration.tv_sec = 1;
// 		(void)timespec_add(&duration, &rem);
// 	}

// 	timeout = timespec_to_timeout(&duration, NULL);
// 	end = sys_timepoint_calc(timeout);
// 	do {
// 		(void)k_sleep(timeout);
// 		timeout = sys_timepoint_timeout(end);
// 	} while (!K_TIMEOUT_EQ(timeout, K_NO_WAIT));

// 	if (update_rmtp) {
// 		*rmtp = (struct timespec){
// 			.tv_sec = 0,
// 			.tv_nsec = 0,
// 		};
// 	}

// 	return 0;
// }
