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

#ifndef DEPS_SSG_MICRO_RUNTIME_WASM_POC_APP_LIBS_NATIVE_INTERFACE_SHARED_UTILS_H_
#define DEPS_SSG_MICRO_RUNTIME_WASM_POC_APP_LIBS_NATIVE_INTERFACE_SHARED_UTILS_H_

#include "native_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FMT_ATTR_CONTAINER  99
#define FMT_APP_RAW_BINARY  98

typedef struct request {
    // message id
    uint32 mid;

    // url of the request
    char *url;

    // action of the request, can be PUT/GET/POST/DELETE
    int action;

    // payload format, currently only support attr_container_t type
    int fmt;

    // payload of the request, currently only support attr_container_t type
    void *payload;

    int payload_len;

    unsigned long sender;
} request_t;

typedef struct response {
    // message id
    uint32 mid;

    // status of the response
    int status;

    // payload format
    int fmt;

    // payload of the response,
    void *payload;

    int payload_len;

    unsigned long reciever;
} response_t;

int check_url_start(const char* url, int url_len, const char * leading_str);
bool match_url(char * pattern, char * matched);
char * find_key_value(char * buffer, int buffer_len, char * key, char * value,
        int value_len, char delimiter);

request_t *clone_request(request_t *request);
void request_cleaner(request_t *request);

response_t * clone_response(response_t * response);
void response_cleaner(response_t * response);

response_t * set_response(response_t * response, int status, int fmt,
        const char *payload, int payload_len);
response_t * make_response_for_request(request_t * request,
        response_t * response);

request_t * init_request(request_t * request, char *url, int action, int fmt,
        void *payload, int payload_len);

char * pack_request(request_t *request, int * size);
request_t * unpack_request(char * packet, int size, request_t * request);
char * pack_response(response_t *response, int * size);
response_t * unpack_response(char * packet, int size, response_t * response);
void free_req_resp_packet(char * packet);

#ifdef __cplusplus
}
#endif

#endif /* DEPS_SSG_MICRO_RUNTIME_WASM_POC_APP_LIBS_NATIVE_INTERFACE_SHARED_UTILS_H_ */
