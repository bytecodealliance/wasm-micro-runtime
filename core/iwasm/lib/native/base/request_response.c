/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "app_manager_export.h"
#include "coap_ext.h"
#include "wasm_export.h"

extern void module_request_handler(request_t *request, void *user_data);

bool
wasm_response_send(wasm_module_inst_t module_inst,
                   int32 buffer_offset, int size)
{
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

void
wasm_register_resource(wasm_module_inst_t module_inst, int32 url_offset)
{
    char *url = NULL;

    if (!validate_app_str_addr(url_offset))
        return;

    url = addr_app_to_native(url_offset);

    if (url != NULL) {
        unsigned int mod_id = app_manager_get_module_id(Module_WASM_App,
                                                        module_inst);
        am_register_resource(url, module_request_handler, mod_id);
    }
}

void
wasm_post_request(wasm_module_inst_t module_inst,
                  int32 buffer_offset, int size)
{
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
        unsigned int mod_id = app_manager_get_module_id(Module_WASM_App,
                                                        module_inst);
        req->sender = mod_id;

        if (req->action == COAP_EVENT) {
            am_publish_event(req);
            return;
        }

        am_dispatch_request(req);
    }
}

void
wasm_sub_event(wasm_module_inst_t module_inst, int32 url_offset)
{
    char *url = NULL;

    if (!validate_app_str_addr(url_offset))
        return;

    url = addr_app_to_native(url_offset);

    if (url != NULL) {
        unsigned int mod_id = app_manager_get_module_id(Module_WASM_App,
                                                        module_inst);

        am_register_event(url, mod_id);
    }
}

