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

#include "native_interface.h"
#include "app_manager_export.h"
#include "coap_ext.h"
#include "wasm_export.h"

extern void module_request_handler(request_t *request, void *user_data);

bool wasm_response_send(int32 buffer_offset, int size)
{
    wasm_module_inst_t module_inst = get_module_inst();
    char *buffer = NULL;

    if (!validate_app_addr(buffer_offset, size))
        return false;

    buffer = addr_app_to_native(buffer_offset);

    if (buffer != NULL) {
        response_t response[1];

        if (NULL == unpack_response(buffer, size, response))
            return false;

        am_send_response(response);

        return true;
    }

    return false;
}

void wasm_register_resource(int32 url_offset)
{
    wasm_module_inst_t module_inst = get_module_inst();
    char *url = NULL;

    if (!validate_app_addr(url_offset, 1))
        return;

    url = addr_app_to_native(url_offset);

    if (url != NULL) {
        unsigned int mod_id = app_manager_get_module_id(Module_WASM_App);
        am_register_resource(url, module_request_handler, mod_id);
    }
}

void wasm_post_request(int32 buffer_offset, int size)
{
    wasm_module_inst_t module_inst = get_module_inst();
    char *buffer = NULL;

    if (!validate_app_addr(buffer_offset, size))
        return;

    buffer = addr_app_to_native(buffer_offset);

    if (buffer != NULL) {
        request_t req[1];

        if (!unpack_request(buffer, size, req))
            return;

        // TODO: add permission check, ensure app can't do harm

        // set sender to help dispatch the response to the sender ap
        unsigned int mod_id = app_manager_get_module_id(Module_WASM_App);
        req->sender = mod_id;

        if (req->action == COAP_EVENT) {
            am_publish_event(req);
            return;
        }

        am_dispatch_request(req);
    }
}

void wasm_sub_event(int32 url_offset)
{
    wasm_module_inst_t module_inst = get_module_inst();
    char *url = NULL;

    if (!validate_app_addr(url_offset, 1))
        return;

    url = addr_app_to_native(url_offset);

    if (url != NULL) {
        unsigned int mod_id = app_manager_get_module_id(Module_WASM_App);

        am_register_event(url, mod_id);
    }
}

