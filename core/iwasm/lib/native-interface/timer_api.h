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

#ifndef _TIMER_API_H_
#define _TIMER_API_H_

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int timer_id_t;

timer_id_t
wasm_create_timer(int interval, bool is_period, bool auto_start);

void
wasm_timer_destory(timer_id_t timer_id);

void
wasm_timer_cancel(timer_id_t timer_id);

void
wasm_timer_restart(timer_id_t timer_id, int interval);

uint32
wasm_get_sys_tick_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* end of _TIMER_API_H_ */

