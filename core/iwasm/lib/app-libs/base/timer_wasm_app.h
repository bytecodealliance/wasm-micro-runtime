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

#ifndef _AEE_TIMER_H_
#define _AEE_TIMER_H_

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* board producer define user_timer */
struct user_timer;
typedef struct user_timer * user_timer_t;

/**
 * @typedef on_user_timer_update_f
 *
 * @brief Define the signature of callback function for API api_timer_create().
 *
 * @param timer the timer
 *
 * @see api_timer_create
 */
typedef void (*on_user_timer_update_f)(user_timer_t timer);

/*
 *****************
 * Timer APIs
 *****************
 */

/**
 * @brief Create timer.
 *
 * @param interval timer interval
 * @param is_period whether the timer is periodic
 * @param auto_start whether start the timer immediately after created
 * @param on_timer_update callback function called when timer expired
 *
 * @return the timer created if success, NULL otherwise
 */
user_timer_t api_timer_create(int interval, bool is_period, bool auto_start,
        on_user_timer_update_f on_timer_update);

/**
 * @brief Cancel timer.
 *
 * @param timer the timer to cancel
 */
void api_timer_cancel(user_timer_t timer);

/**
 * @brief Restart timer.
 *
 * @param timer the timer to cancel
 * @param interval the timer interval
 */
void api_timer_restart(user_timer_t timer, int interval);

#ifdef __cplusplus
}
#endif

#endif
