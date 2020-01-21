/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _REQ_RESP_API_H_
#define _REQ_RESP_API_H_

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
wasm_response_send(int32 buf_offset, int size);

void
wasm_register_resource(int32 url_offset);

void
wasm_post_request(int32 buf_offset, int size);

void
wasm_sub_event(int32 url_offset);

#ifdef __cplusplus
}
#endif

#endif /* end of _REQ_RESP_API_H_ */

