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

#ifndef COAP_EXTENSION_COAP_EXT_H_
#define COAP_EXTENSION_COAP_EXT_H_

#include "er-coap.h"
#include "shared_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COAP_EVENT (COAP_DELETE + 2)

char * coap_get_full_url_alloc(coap_packet_t * request);

coap_status_t coap_parse_message_tcp(void *packet, uint8_t *data,
        uint32_t data_len);

int coap_serialize_message_tcp(void *packet, uint8_t ** buffer_out);
int coap_set_payload_tcp(void *packet, const void *payload, size_t length);
uint8_t coap_is_request(coap_packet_t * coap_message);

uint16_t coap_find_mid(uint8_t *buffer);
uint8_t coap_find_code(uint8_t *buffer);
void coap_change_mid(uint8_t *buffer, uint16_t id);

int add_resource_handler(coap_context_t * coap_ctx,
        coap_resource_handler_t * handler);
uint32_t check_blockwise_timeout_ms(coap_context_t * coap_ctx, int timeout_sec);

int convert_coap_packet_to_request(coap_packet_t *packet, request_t *request);
void convert_coap_packet_to_response(coap_packet_t *packet,
        response_t *response);

void convert_response_to_coap_packet(response_t * response,
        coap_packet_t * packet);
void convert_request_to_coap_packet(request_t * req, coap_packet_t * packet);

#ifdef __cplusplus
}
#endif
#endif /* COAP_EXTENSION_COAP_EXT_H_ */
