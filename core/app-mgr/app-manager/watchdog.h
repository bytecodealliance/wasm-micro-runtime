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


#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_

#include "app_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
watchdog_timer_init(module_data *module_data);

void
watchdog_timer_destroy(watchdog_timer *wd_timer);

void
watchdog_timer_start(watchdog_timer *wd_timer);

void
watchdog_timer_stop(watchdog_timer *wd_timer);

watchdog_timer*
app_manager_get_watchdog_timer(void *timer);

bool
watchdog_startup();

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* _WATCHDOG_H_ */
