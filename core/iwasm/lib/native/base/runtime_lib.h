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

#ifndef LIB_BASE_RUNTIME_LIB_H_
#define LIB_BASE_RUNTIME_LIB_H_

#include "native_interface.h"

#include "runtime_timer.h"

void init_wasm_timer();
timer_ctx_t get_wasm_timer_ctx();
timer_ctx_t create_wasm_timer_ctx(unsigned int module_id, int prealloc_num);
void destory_module_timer_ctx(unsigned int module_id);

#endif /* LIB_BASE_RUNTIME_LIB_H_ */
