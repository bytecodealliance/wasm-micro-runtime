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

#ifndef _AEE_REQUEST_H_
#define _AEE_REQUEST_H_

#include "native_interface.h"
#include "shared_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

bool is_event_type(request_t * req);

typedef void (*request_handler_f)(request_t *);
typedef void (*response_handler_f)(response_t *, void *);

// Request APIs
bool api_register_resource_handler(const char *url, request_handler_f);
void api_send_request(request_t * request, response_handler_f response_handler,
        void * user_data);

void api_response_send(response_t *response);

// event API
bool api_publish_event(const char *url, int fmt, void *payload,
        int payload_len);

bool api_subscribe_event(const char * url, request_handler_f handler);

#ifdef __cplusplus
}
#endif

#endif
