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

#ifndef CONNECTION_API_H_
#define CONNECTION_API_H_
#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32 wasm_open_connection(int32 name_offset, int32 args_offset, uint32 len);

void wasm_close_connection(uint32 handle);

int wasm_send_on_connection(uint32 handle, int32 data_offset, uint32 len);

bool wasm_config_connection(uint32 handle, int32 cfg_offset, uint32 len);

#ifdef __cplusplus
}
#endif


#endif /* CONNECTION_API_H_ */
