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

#ifndef GUI_API_H_
#define GUI_API_H_
#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

void wasm_obj_native_call(int32 func_id, uint32 argv_offset, uint32 argc);
void wasm_btn_native_call(int32 func_id, uint32 argv_offset, uint32 argc);
void wasm_label_native_call(int32 func_id, uint32 argv_offset, uint32 argc);
void wasm_cb_native_call(int32 func_id, uint32 argv_offset, uint32 argc);
void wasm_list_native_call(int32 func_id, uint32 argv_offset, uint32 argc);


#ifdef __cplusplus
}
#endif


#endif /* GUI_API_H_ */
