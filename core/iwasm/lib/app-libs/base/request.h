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

/**
 * @typedef request_handler_f
 *
 * @brief Define the signature of callback function for API
 * api_register_resource_handler() to handle request or for API
 * api_subscribe_event() to handle event.
 *
 * @param request pointer of the request to be handled
 *
 * @see api_register_resource_handler
 * @see api_subscribe_event
 */
typedef void (*request_handler_f)(request_t *request);

/**
 * @typedef response_handler_f
 *
 * @brief Define the signature of callback function for API
 * api_send_request() to handle response of a request.
 *
 * @param response pointer of the response to be handled
 * @param user_data user data associated with the request which is set when
 * calling api_send_request().
 *
 * @see api_send_request
 */
typedef void (*response_handler_f)(response_t *response, void *user_data);


/*
 *****************
 * Request APIs
 *****************
 */

/**
 * @brief Register resource.
 *
 * @param url url of the resource
 * @param handler callback function to handle the request to the resource
 *
 * @return true if success, false otherwise
 */
bool api_register_resource_handler(const char *url, request_handler_f handler);

/**
 * @brief Send request asynchronously.
 *
 * @param request pointer of the request to be sent
 * @param response_handler callback function to handle the response
 * @param user_data user data
 */
void api_send_request(request_t * request, response_handler_f response_handler,
        void * user_data);

/**
 * @brief Send response.
 *
 * @param response pointer of the response to be sent
 *
 * @par
 * @code
 * void res1_handler(request_t *request)
 * {
 *     response_t response[1];
 *     make_response_for_request(request, response);
 *     set_response(response, DELETED_2_02, 0, NULL, 0);
 *     api_response_send(response);
 * }
 * @endcode
 */
void api_response_send(response_t *response);


/*
 *****************
 * Event APIs
 *****************
 */

/**
 * @brief Publish an event.
 *
 * @param url url of the event
 * @param fmt format of the event payload
 * @param payload payload of the event
 * @param payload_len length in bytes of the event payload
 *
 * @return true if success, false otherwise
 */
bool api_publish_event(const char *url, int fmt, void *payload,
        int payload_len);


/**
 * @brief Subscribe an event.
 *
 * @param url url of the event
 * @param handler callback function to handle the event.
 *
 * @return true if success, false otherwise
 */
bool api_subscribe_event(const char * url, request_handler_f handler);

#ifdef __cplusplus
}
#endif

#endif
