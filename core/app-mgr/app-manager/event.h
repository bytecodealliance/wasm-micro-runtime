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

#ifndef _EVENT_H_
#define _EVENT_H_

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle event request from host agent
 *
 * @param code the coap packet code
 * @param event_url the event url
 *
 * @return true if success, false otherwise
 */
bool
event_handle_event_request(uint8_t code, const char *event_url,
        uint32_t register);

/**
 * Test whether the event is registered
 *
 * @param event_url the event url
 *
 * @return true for registered, false for not registered
 */
bool
event_is_registered(const char *event_url);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* _EVENT_H_ */
