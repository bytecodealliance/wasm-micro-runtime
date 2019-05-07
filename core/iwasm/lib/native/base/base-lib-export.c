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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib-export.h"

#ifdef WASM_ENABLE_BASE_LIB
#include "base-lib-export.h"
#endif

static NativeSymbol extended_native_symbol_defs[] = {
/* TODO: use macro EXPORT_WASM_API() or EXPORT_WASM_API2() to
 add functions to register. */

#ifdef WASM_ENABLE_BASE_LIB
        EXPORT_WASM_API(wasm_register_resource),
        EXPORT_WASM_API(wasm_response_send),
        EXPORT_WASM_API(wasm_post_request),
        EXPORT_WASM_API(wasm_sub_event),
        EXPORT_WASM_API(wasm_create_timer),
        EXPORT_WASM_API(wasm_timer_destory),
        EXPORT_WASM_API(wasm_timer_cancel),
        EXPORT_WASM_API(wasm_timer_restart),
        EXPORT_WASM_API(wasm_get_sys_tick_ms),
#endif
    };

int get_base_lib_export_apis(NativeSymbol **p_base_lib_apis)
{
    *p_base_lib_apis = extended_native_symbol_defs;
    return sizeof(extended_native_symbol_defs) / sizeof(NativeSymbol);
}

