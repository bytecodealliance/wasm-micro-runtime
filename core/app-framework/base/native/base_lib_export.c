/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib_export.h"
#include "base_lib_export.h"

static NativeSymbol extended_native_symbol_defs[] = {
    /* TODO: use macro EXPORT_WASM_API() or EXPORT_WASM_API2() to
       add functions to register. */
    EXPORT_WASM_API(wasm_register_resource),
    EXPORT_WASM_API(wasm_response_send),
    EXPORT_WASM_API(wasm_post_request),
    EXPORT_WASM_API(wasm_sub_event),
    EXPORT_WASM_API(wasm_create_timer),
    EXPORT_WASM_API(wasm_timer_destroy),
    EXPORT_WASM_API(wasm_timer_cancel),
    EXPORT_WASM_API(wasm_timer_restart),
    EXPORT_WASM_API(wasm_get_sys_tick_ms),
};

int get_base_lib_export_apis(NativeSymbol **p_base_lib_apis)
{
    *p_base_lib_apis = extended_native_symbol_defs;
    return sizeof(extended_native_symbol_defs) / sizeof(NativeSymbol);
}

