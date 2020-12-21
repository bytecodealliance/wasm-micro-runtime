/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _GUI_API_H_
#define _GUI_API_H_

#include "bh_platform.h"
#include "bi-inc/wgl_shared_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

void
wasm_obj_native_call(int32 func_id, uint32 *argv, uint32 argc);

void
wasm_btn_native_call(int32 func_id, uint32 *argv, uint32 argc);

void
wasm_label_native_call(int32 func_id, uint32 *argv, uint32 argc);

void
wasm_cb_native_call(int32 func_id, uint32 *argv, uint32 argc);

void
wasm_list_native_call(int32 func_id, uint32 *argv, uint32 argc);


#ifdef __cplusplus
}
#endif


#endif /* end of _GUI_API_H_ */
