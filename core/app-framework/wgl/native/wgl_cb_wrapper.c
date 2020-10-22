/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "lvgl.h"
#include "wasm_export.h"
#include "native_interface.h"
#include "module_wasm_app.h"
#include "wgl_native_utils.h"

/* -------------------------------------------------------------------------
 * Label widget native function wrappers
 * -------------------------------------------------------------------------*/
DEFINE_WGL_NATIVE_WRAPPER(lv_cb_create_wrapper)
{
    int32 res;
    wgl_native_return_type(int32);
    wgl_native_get_arg(uint32, par_obj_id);
    wgl_native_get_arg(uint32, copy_obj_id);
    wasm_module_inst_t module_inst = get_module_inst(exec_env);

    res = wgl_native_wigdet_create(WIDGET_TYPE_CB, par_obj_id, copy_obj_id, module_inst);
    wgl_native_set_return(res);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_cb_set_text_wrapper)
{
    char *text;
    wgl_native_get_arg(lv_obj_t *, cb);
    wgl_native_get_arg(uint32, text_offset);
    wgl_native_get_arg(uint32, text_len);
    wasm_module_inst_t module_inst = get_module_inst(exec_env);

    if (!validate_app_addr(text_offset, text_len)
        || !(text = addr_app_to_native(text_offset)))
        return;

    lv_cb_set_text(cb, text);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_cb_set_static_text_wrapper)
{
    char *text;
    wgl_native_get_arg(lv_obj_t *, cb);
    wgl_native_get_arg(uint32, text_offset);
    wgl_native_get_arg(uint32, text_len);
    wasm_module_inst_t module_inst = get_module_inst(exec_env);

    if (!validate_app_addr(text_offset, text_len)
        || !(text = addr_app_to_native(text_offset)))
        return;

    lv_cb_set_static_text(cb, text);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_cb_get_text_length_wrapper)
{
    const char *text;
    wgl_native_return_type(int32);
    wgl_native_get_arg(lv_obj_t *, cb);

    (void)exec_env;

    text = lv_cb_get_text(cb);
    wgl_native_set_return(text ? strlen(text): 0);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_cb_get_text_wrapper)
{
    const char *text;
    char *buffer;
    wgl_native_return_type(uint32);
    wgl_native_get_arg(lv_obj_t *, cb);
    wgl_native_get_arg(uint32, buffer_offset);
    wgl_native_get_arg(int, buffer_len);
    wasm_module_inst_t module_inst = get_module_inst(exec_env);

    if (!validate_app_addr(buffer_offset, buffer_len)
        || !(buffer = addr_app_to_native(buffer_offset)))
        return;

    if ((text = lv_cb_get_text(cb))) {
        strncpy(buffer, text, buffer_len - 1);
        buffer[buffer_len - 1] = '\0';
    }

    wgl_native_set_return(buffer_offset);
}

static WGLNativeFuncDef cb_native_func_defs[] = {
    { CB_FUNC_ID_CREATE,            lv_cb_create_wrapper,            2,  false },
    { CB_FUNC_ID_SET_TEXT,          lv_cb_set_text_wrapper,          3,  true },
    { CB_FUNC_ID_SET_STATIC_TEXT,   lv_cb_set_static_text_wrapper,   3,  true },
    { CB_FUNC_ID_GET_TEXT_LENGTH,   lv_cb_get_text_length_wrapper,   1,  true },
    { CB_FUNC_ID_GET_TEXT,          lv_cb_get_text_wrapper,          3,  true },
};

/*************** Native Interface to Wasm App ***********/
void
wasm_cb_native_call(wasm_exec_env_t exec_env,
                    int32 func_id, uint32 *argv, uint32 argc)
{
    uint32 size = sizeof(cb_native_func_defs) / sizeof(WGLNativeFuncDef);

    wgl_native_func_call(exec_env,
                         cb_native_func_defs,
                         size,
                         func_id,
                         argv,
                         argc);
}
