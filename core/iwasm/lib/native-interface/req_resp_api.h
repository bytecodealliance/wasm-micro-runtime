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

#ifndef _REQ_RESP_API_H_
#define _REQ_RESP_API_H_

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
wasm_response_send(const char *buf, int size);

void
wasm_register_resource(const char *url);

void
wasm_post_request(const char *buf, int size);

void
wasm_sub_event(const char *url);

#ifdef __cplusplus
}
#endif

#endif /* end of _REQ_RESP_API_H_ */

