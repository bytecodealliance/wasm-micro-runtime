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
#include "lib_export.h"
#include "bh_platform.h"
#include "wasm_export.h"

#ifdef WASM_ENABLE_BASE_LIB
#include "base_lib_export.h"
#endif

static uint64
wasm_runtime_get_current_module_inst_wrapper(wasm_module_inst_t module_inst)
{
    return (uint64)(uintptr_t)module_inst;
}

static bool
wasm_runtime_validate_app_addr_wrapper(wasm_module_inst_t module_inst,
                                       uint32 inst_part0, uint32 inst_part1,
                                       int32 app_offset, uint32 size)
{
    bool ret;
    union { uint64 u64; uint32 parts[2]; } inst;

    inst.parts[0] = inst_part0;
    inst.parts[1] = inst_part1;

    if (inst.u64 != (uint64)(uintptr_t)module_inst) {
        printf("Invalid module instance\n");
        return false;
    }

    ret = wasm_runtime_validate_app_addr(module_inst, app_offset, size);
    if (!ret)
        wasm_runtime_clear_exception(module_inst);
    return ret;
}

static bool
wasm_runtime_validate_native_addr_wrapper(wasm_module_inst_t module_inst,
                                          uint32 inst_part0, uint32 inst_part1,
                                          uint32 native_ptr_part0,
                                          uint32 native_ptr_part1,
                                          uint32 size)
{
    bool ret;
    union { uint64 u64; uint32 parts[2]; } inst;
    union { uint64 u64; uint32 parts[2]; } native_ptr;

    inst.parts[0] = inst_part0;
    inst.parts[1] = inst_part1;

    if (inst.u64 != (uint64)(uintptr_t)module_inst) {
        printf("Invalid module instance\n");
        return false;
    }

    native_ptr.parts[0] = native_ptr_part0;
    native_ptr.parts[1] = native_ptr_part1;
    ret  = wasm_runtime_validate_native_addr(module_inst,
                                             (void*)(uintptr_t)native_ptr.u64,
                                             size);
    if (!ret)
        wasm_runtime_clear_exception(module_inst);
    return ret;
}

static uint64
wasm_runtime_addr_app_to_native_wrapper(wasm_module_inst_t module_inst,
                                        uint32 inst_part0, uint32 inst_part1,
                                        int32 app_offset)
{
    union { uint64 u64; uint32 parts[2]; } inst;

    inst.parts[0] = inst_part0;
    inst.parts[1] = inst_part1;

    if (inst.u64 != (uint64)(uintptr_t)module_inst) {
        printf("Invalid module instance\n");
        return 0;
    }
    return (uint64)(uintptr_t)
            wasm_runtime_addr_app_to_native(module_inst, app_offset);
}

static int32
wasm_runtime_addr_native_to_app_wrapper(wasm_module_inst_t module_inst,
                                        uint32 inst_part0, uint32 inst_part1,
                                        uint32 native_ptr_part0,
                                        uint32 native_ptr_part1)
{
    union { uint64 u64; uint32 parts[2]; } inst;
    union { uint64 u64; uint32 parts[2]; } native_ptr;

    inst.parts[0] = inst_part0;
    inst.parts[1] = inst_part1;

     if (inst.u64 != (uint64)(uintptr_t)module_inst) {
         printf("Invalid module instance\n");
         return 0;
     }

    native_ptr.parts[0] = native_ptr_part0;
    native_ptr.parts[1] = native_ptr_part1;
    return wasm_runtime_addr_native_to_app(module_inst,
                                           (void*)(uintptr_t)native_ptr.u64);
}

static NativeSymbol extended_native_symbol_defs[] = {
/* TODO: use macro EXPORT_WASM_API() or EXPORT_WASM_API2() to
 add functions to register. */

#ifdef WASM_ENABLE_BASE_LIB
        EXPORT_WASM_API(wasm_register_resource),
        EXPORT_WASM_API(wasm_response_send),
        EXPORT_WASM_API(wasm_post_request),
        EXPORT_WASM_API(wasm_sub_event),
        EXPORT_WASM_API(wasm_create_timer),
        EXPORT_WASM_API(wasm_timer_destroy),
        EXPORT_WASM_API(wasm_timer_cancel),
        EXPORT_WASM_API(wasm_timer_restart),
        EXPORT_WASM_API(wasm_get_sys_tick_ms),
#endif
        EXPORT_WASM_API2(wasm_runtime_get_current_module_inst),
        EXPORT_WASM_API2(wasm_runtime_validate_app_addr),
        EXPORT_WASM_API2(wasm_runtime_validate_native_addr),
        EXPORT_WASM_API2(wasm_runtime_addr_app_to_native),
        EXPORT_WASM_API2(wasm_runtime_addr_native_to_app),
    };

int get_base_lib_export_apis(NativeSymbol **p_base_lib_apis)
{
    *p_base_lib_apis = extended_native_symbol_defs;
    return sizeof(extended_native_symbol_defs) / sizeof(NativeSymbol);
}

