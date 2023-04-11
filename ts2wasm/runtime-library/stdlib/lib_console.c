/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_export.h"
#include "bh_platform.h"
#include "gc_object.h"
#include "quickjs.h"
#include "dyntype.h"

void *
console_constructor(wasm_exec_env_t exec_env, void *obj)
{
    return obj;
}

void console_log(wasm_exec_env_t exec_env, void *obj)
{
    uint32 i, len;
    WASMObjectRef obj_ref = (WASMObjectRef)obj;
    assert(wasm_obj_is_array_obj(obj_ref));

    WASMArrayObjectRef arr_ref = (WASMArrayObjectRef)obj_ref;
    len = wasm_array_obj_length(arr_ref);
    for (i = 0; i < len; i++) {
        void *addr = wasm_array_obj_elem_addr(arr_ref, i);
        WASMAnyrefObjectRef anyref = *((WASMAnyrefObjectRef *)addr);
        JSValue *js_value = (JSValue *)wasm_anyref_obj_get_value(anyref);
        if ((JS_VALUE_GET_TAG(*js_value) == JS_TAG_EXT_OBJ)
            || (JS_VALUE_GET_TAG(*js_value) == JS_TAG_EXT_FUNC)
            || (JS_VALUE_GET_TAG(*js_value) == JS_TAG_EXT_INFC)) {
            printf("[wasm object]");
        }
        else {
            dyntype_dump_value(dyntype_get_context(), js_value);
        }

        printf(" ");
    }
    printf("\n");
}

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, func_name, signature, NULL }

static NativeSymbol native_symbols[] = {
    REG_NATIVE_FUNC(console_constructor, "(r)r"),
    REG_NATIVE_FUNC(console_log, "(r)"),
    /* TODO */
};
/* clang-format on */

uint32_t
get_lib_console_symbols(char **p_module_name, NativeSymbol **p_native_symbols)
{
    *p_module_name = "env";
    *p_native_symbols = native_symbols;
    return sizeof(native_symbols) / sizeof(NativeSymbol);
}
