/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "native_interface.h"
#include "lvgl.h"
#include "module_wasm_app.h"
#include "wgl_native_utils.h"

/* -------------------------------------------------------------------------
 * Button widget native function wrappers
 * -------------------------------------------------------------------------*/
DEFINE_WGL_NATIVE_WRAPPER(lv_btn_create_wrapper)
{
    int32 res;
    wgl_native_return_type(int32);
    wgl_native_get_arg(uint32, par_obj_id);
    wgl_native_get_arg(uint32, copy_obj_id);
    wasm_module_inst_t module_inst = get_module_inst(exec_env);

    res = wgl_native_wigdet_create(WIDGET_TYPE_BTN, par_obj_id, copy_obj_id,
                                   module_inst);
    wgl_native_set_return(res);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_btn_set_toggle_wrapper)
{
    wgl_native_get_arg(lv_obj_t *, btn);
    wgl_native_get_arg(bool, tgl);

    (void)exec_env;
    lv_btn_set_toggle(btn, tgl);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_btn_set_state_wrapper)
{
    wgl_native_get_arg(lv_obj_t *, btn);
    wgl_native_get_arg(lv_btn_state_t, state);

    (void)exec_env;
    lv_btn_set_state(btn, state);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_btn_set_ink_in_time_wrapper)
{
    wgl_native_get_arg(lv_obj_t *, btn);
    wgl_native_get_arg(uint16_t, time);

    (void)exec_env;
    lv_btn_set_ink_in_time(btn, time);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_btn_set_ink_out_time_wrapper)
{
    wgl_native_get_arg(lv_obj_t *, btn);
    wgl_native_get_arg(uint16_t, time);

    (void)exec_env;
    lv_btn_set_ink_out_time(btn, time);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_btn_set_ink_wait_time_wrapper)
{
    wgl_native_get_arg(lv_obj_t *, btn);
    wgl_native_get_arg(uint16_t, time);

    (void)exec_env;
    lv_btn_set_ink_wait_time(btn, time);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_btn_get_ink_in_time_wrapper)
{
    uint16_t res;
    wgl_native_return_type(uint16_t);
    wgl_native_get_arg(lv_obj_t *, btn);

    (void)exec_env;
    res = lv_btn_get_ink_in_time(btn);
    wgl_native_set_return(res);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_btn_get_ink_out_time_wrapper)
{
    uint16_t res;
    wgl_native_return_type(uint16_t);
    wgl_native_get_arg(lv_obj_t *, btn);

    (void)exec_env;
    res = lv_btn_get_ink_out_time(btn);
    wgl_native_set_return(res);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_btn_get_ink_wait_time_wrapper)
{
    uint16_t res;
    wgl_native_return_type(uint16_t);
    wgl_native_get_arg(lv_obj_t *, btn);

    (void)exec_env;
    res = lv_btn_get_ink_wait_time(btn);
    wgl_native_set_return(res);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_btn_get_state_wrapper)
{
    lv_btn_state_t res;
    wgl_native_return_type(lv_btn_state_t);
    wgl_native_get_arg(lv_obj_t *, btn);

    (void)exec_env;
    res = lv_btn_get_state(btn);
    wgl_native_set_return(res);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_btn_get_toggle_wrapper)
{
    bool res;
    wgl_native_return_type(bool);
    wgl_native_get_arg(lv_obj_t *, btn);

    (void)exec_env;
    res = lv_btn_get_toggle(btn);
    wgl_native_set_return(res);
}

DEFINE_WGL_NATIVE_WRAPPER(lv_btn_toggle_wrapper)
{
    wgl_native_get_arg(lv_obj_t *, btn);

    (void)exec_env;
    lv_btn_toggle(btn);
}

/* clang-format off */
static WGLNativeFuncDef btn_native_func_defs[] = {
    { BTN_FUNC_ID_CREATE, lv_btn_create_wrapper, 2, false },
    { BTN_FUNC_ID_SET_TOGGLE, lv_btn_set_toggle_wrapper, 2, true },
    { BTN_FUNC_ID_SET_STATE, lv_btn_set_state_wrapper, 2, true },
    { BTN_FUNC_ID_SET_INK_IN_TIME, lv_btn_set_ink_in_time_wrapper, 2, true },
    { BTN_FUNC_ID_SET_INK_OUT_TIME, lv_btn_set_ink_out_time_wrapper, 2, true },
    { BTN_FUNC_ID_SET_INK_WAIT_TIME, lv_btn_set_ink_wait_time_wrapper, 2, true },
    { BTN_FUNC_ID_GET_INK_IN_TIME, lv_btn_get_ink_in_time_wrapper, 1, true },
    { BTN_FUNC_ID_GET_INK_OUT_TIME, lv_btn_get_ink_out_time_wrapper, 1, true },
    { BTN_FUNC_ID_GET_INK_WAIT_TIME, lv_btn_get_ink_wait_time_wrapper, 1, true },
    { BTN_FUNC_ID_GET_STATE, lv_btn_get_state_wrapper, 1, true },
    { BTN_FUNC_ID_GET_TOGGLE, lv_btn_get_toggle_wrapper, 1, true },
    { BTN_FUNC_ID_TOGGLE, lv_btn_toggle_wrapper, 1, true },
};
/* clang-format on */

/*************** Native Interface to Wasm App ***********/
void
wasm_btn_native_call(wasm_exec_env_t exec_env, int32 func_id, uint32 *argv,
                     uint32 argc)
{
    uint32 size = sizeof(btn_native_func_defs) / sizeof(WGLNativeFuncDef);

    wgl_native_func_call(exec_env, btn_native_func_defs, size, func_id, argv,
                         argc);
}
