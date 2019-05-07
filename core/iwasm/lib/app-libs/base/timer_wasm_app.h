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

//TODO:
#define bh_queue_t void

/* board producer define user_timer */
struct user_timer;
typedef struct user_timer * user_timer_t;

// Timer APIs
user_timer_t api_timer_create(int interval, bool is_period, bool auto_start,
        void (*on_user_timer_update)(user_timer_t));
void api_timer_cancel(user_timer_t timer);
void api_timer_restart(user_timer_t timer, int interval);

#ifdef __cplusplus
}
#endif

#endif
