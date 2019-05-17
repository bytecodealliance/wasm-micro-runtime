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

#ifndef DEPS_APP_MGR_APP_MGR_SHARED_HOST_LINK_H_
#define DEPS_APP_MGR_APP_MGR_SHARED_HOST_LINK_H_

typedef enum LINK_MSG_TYPE {
    COAP_TCP_RAW = 0,
    COAP_UDP_RAW = 1,
    REQUEST_PACKET,
    RESPONSE_PACKET,
    INSTALL_WASM_BYTECODE_APP,
    CBOR_GENERIC = 30,

    LINK_MSG_TYPE_MAX = 50
} LINK_MSG_TYPE;

/* Link message, or message between host and app manager */
typedef struct bh_link_msg_t {
    /* 2 bytes leading */
    uint16_t leading_bytes;
    /* message type, must be COAP_TCP or COAP_UDP */
    uint16_t message_type;
    /* size of payload */
    uint32_t payload_size;
    char *payload;
} bh_link_msg_t;

#endif /* DEPS_APP_MGR_APP_MGR_SHARED_HOST_LINK_H_ */
