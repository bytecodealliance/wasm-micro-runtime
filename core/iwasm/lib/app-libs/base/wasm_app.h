/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

#ifndef _LIB_AEE_H_
#define _LIB_AEE_H_

#include "shared_utils.h"
#include "attr_container.h"
#include "request.h"
#include "sensor.h"
#include "connection.h"
#include "timer_wasm_app.h"

#if ENABLE_WGL != 0
#include "wgl.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* CoAP request method codes */
typedef enum {
    COAP_GET = 1,
    COAP_POST,
    COAP_PUT,
    COAP_DELETE,
    COAP_EVENT = (COAP_DELETE + 2)
} coap_method_t;

/* CoAP response codes */
typedef enum {
    NO_ERROR = 0,

    CREATED_2_01 = 65, /* CREATED */
    DELETED_2_02 = 66, /* DELETED */
    VALID_2_03 = 67, /* NOT_MODIFIED */
    CHANGED_2_04 = 68, /* CHANGED */
    CONTENT_2_05 = 69, /* OK */
    CONTINUE_2_31 = 95, /* CONTINUE */

    BAD_REQUEST_4_00 = 128, /* BAD_REQUEST */
    UNAUTHORIZED_4_01 = 129, /* UNAUTHORIZED */
    BAD_OPTION_4_02 = 130, /* BAD_OPTION */
    FORBIDDEN_4_03 = 131, /* FORBIDDEN */
    NOT_FOUND_4_04 = 132, /* NOT_FOUND */
    METHOD_NOT_ALLOWED_4_05 = 133, /* METHOD_NOT_ALLOWED */
    NOT_ACCEPTABLE_4_06 = 134, /* NOT_ACCEPTABLE */
    PRECONDITION_FAILED_4_12 = 140, /* BAD_REQUEST */
    REQUEST_ENTITY_TOO_LARGE_4_13 = 141, /* REQUEST_ENTITY_TOO_LARGE */
    UNSUPPORTED_MEDIA_TYPE_4_15 = 143, /* UNSUPPORTED_MEDIA_TYPE */

    INTERNAL_SERVER_ERROR_5_00 = 160, /* INTERNAL_SERVER_ERROR */
    NOT_IMPLEMENTED_5_01 = 161, /* NOT_IMPLEMENTED */
    BAD_GATEWAY_5_02 = 162, /* BAD_GATEWAY */
    SERVICE_UNAVAILABLE_5_03 = 163, /* SERVICE_UNAVAILABLE */
    GATEWAY_TIMEOUT_5_04 = 164, /* GATEWAY_TIMEOUT */
    PROXYING_NOT_SUPPORTED_5_05 = 165, /* PROXYING_NOT_SUPPORTED */

    /* Erbium errors */
    MEMORY_ALLOCATION_ERROR = 192, PACKET_SERIALIZATION_ERROR,

    /* Erbium hooks */
    MANUAL_RESPONSE, PING_RESPONSE
} coap_status_t;

#ifdef __cplusplus
}
#endif

#endif /* end of _LIB_AEE_H_ */
