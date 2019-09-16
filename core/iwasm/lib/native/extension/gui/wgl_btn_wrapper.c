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

#include "native_interface.h"
#include "lvgl.h"
#include "module_wasm_app.h"
#include "wgl_native_utils.h"

/* -------------------------------------------------------------------------
 * Button widget native function wrappers
 * -------------------------------------------------------------------------*/
static int32 _btn_create(lv_obj_t *par, lv_obj_t *copy)
{
    return wgl_native_wigdet_create(WIDGET_TYPE_BTN, par, copy);
}

static WGLNativeFuncDef btn_native_func_defs[] = {
    { BTN_FUNC_ID_CREATE, _btn_create, HAS_RET, 2, {0 | NULL_OK, 1 | NULL_OK, -1},  {-1} },
    { BTN_FUNC_ID_SET_TOGGLE, lv_btn_set_toggle, NO_RET, 2, {0, -1}, {-1} },
    { BTN_FUNC_ID_SET_STATE, lv_btn_set_state, NO_RET, 2, {0, -1}, {-1} },
//    { BTN_FUNC_ID_SET_STYLE, _btn_set_style, NO_RET, 2, {0, -1}, {-1} },
    { BTN_FUNC_ID_SET_INK_IN_TIME, lv_btn_set_ink_in_time, NO_RET, 2, {0, -1}, {-1} },
    { BTN_FUNC_ID_SET_INK_OUT_TIME, lv_btn_set_ink_out_time, NO_RET, 2, {0, -1}, {-1} },
    { BTN_FUNC_ID_SET_INK_WAIT_TIME, lv_btn_set_ink_wait_time, NO_RET, 2, {0, -1}, {-1} },
    { BTN_FUNC_ID_GET_INK_IN_TIME, lv_btn_get_ink_in_time, HAS_RET, 1, {0, -1}, {-1} },
    { BTN_FUNC_ID_GET_INK_OUT_TIME, lv_btn_get_ink_out_time, HAS_RET, 1, {0, -1}, {-1} },
    { BTN_FUNC_ID_GET_INK_WAIT_TIME, lv_btn_get_ink_wait_time, HAS_RET, 1, {0, -1}, {-1} },
    { BTN_FUNC_ID_GET_STATE, lv_btn_get_state, HAS_RET, 1, {0, -1}, {-1} },
    { BTN_FUNC_ID_GET_TOGGLE, lv_btn_get_toggle, HAS_RET, 1, {0, -1}, {-1} },
    { BTN_FUNC_ID_TOGGLE, lv_btn_toggle, NO_RET, 1, {0, -1}, {-1} },

};

/*************** Native Interface to Wasm App ***********/
void
wasm_btn_native_call(wasm_module_inst_t module_inst,
                     int32 func_id, uint32 argv_offset, uint32 argc)
{
    uint32 size = sizeof(btn_native_func_defs) / sizeof(WGLNativeFuncDef);

    wgl_native_func_call(module_inst,
                         btn_native_func_defs,
                         size,
                         func_id,
                         argv_offset,
                         argc);
}
