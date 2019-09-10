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

uint32
wasm_open_connection(const char *name, char *args_buf, uint32 args_buf_len);

void
wasm_close_connection(uint32 handle);

int
wasm_send_on_connection(uint32 handle, const char *data, uint32 data_len);

bool
wasm_config_connection(uint32 handle, const char *cfg_buf, uint32 cfg_buf_len);

#ifdef __cplusplus
}
#endif


#endif /* end of CONNECTION_API_H_ */
