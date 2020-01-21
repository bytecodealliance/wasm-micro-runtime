/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef CONNECTION_API_H_
#define CONNECTION_API_H_

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32
wasm_open_connection(int32 name_offset, int32 args_buf_offset, uint32 args_buf_len);

void
wasm_close_connection(uint32 handle);

int
wasm_send_on_connection(uint32 handle, int32 data_offset, uint32 data_len);

bool
wasm_config_connection(uint32 handle, int32 cfg_buf_offset, uint32 cfg_buf_len);

#ifdef __cplusplus
}
#endif


#endif /* end of CONNECTION_API_H_ */
