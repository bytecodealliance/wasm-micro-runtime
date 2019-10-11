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

#include "wgl.h"
#include "bh_platform.h"
#include "gui_api.h"

#define ARGC sizeof(argv)/sizeof(uint32)
#define CALL_BTN_NATIVE_FUNC(id) wasm_btn_native_call(id, argv, ARGC)

wgl_obj_t wgl_btn_create(wgl_obj_t par, wgl_obj_t copy)
{
    uint32 argv[2] = {0};

    argv[0] = (uint32)par;
    argv[1] = (uint32)copy;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_CREATE);
    return (wgl_obj_t)argv[0];
}

void wgl_btn_set_toggle(wgl_obj_t btn, bool tgl)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)btn;
    argv[1] = tgl;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_SET_TOGGLE);
}

void wgl_btn_set_state(wgl_obj_t btn, wgl_btn_state_t state)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)btn;
    argv[1] = state;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_SET_STATE);
}

void wgl_btn_toggle(wgl_obj_t btn)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)btn;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_TOGGLE);
}

void wgl_btn_set_ink_in_time(wgl_obj_t btn, uint16_t time)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)btn;
    argv[1] = time;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_SET_INK_IN_TIME);
}

void wgl_btn_set_ink_wait_time(wgl_obj_t btn, uint16_t time)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)btn;
    argv[1] = time;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_SET_INK_WAIT_TIME);
}

void wgl_btn_set_ink_out_time(wgl_obj_t btn, uint16_t time)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)btn;
    argv[1] = time;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_SET_INK_OUT_TIME);
}

//void wgl_btn_set_style(wgl_obj_t btn, wgl_btn_style_t type, const wgl_style_t * style)
//{
//    //TODO: pack style
//    //wasm_btn_set_style(btn, type, style);
//}
//
wgl_btn_state_t wgl_btn_get_state(const wgl_obj_t btn)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)btn;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_GET_STATE);
    return (wgl_btn_state_t)argv[0];
}

bool wgl_btn_get_toggle(const wgl_obj_t btn)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)btn;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_GET_TOGGLE);
    return (bool)argv[0];
}

uint16_t wgl_btn_get_ink_in_time(const wgl_obj_t btn)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)btn;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_GET_INK_IN_TIME);
    return (uint16_t)argv[0];
}

uint16_t wgl_btn_get_ink_wait_time(const wgl_obj_t btn)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)btn;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_GET_INK_WAIT_TIME);
    return (uint16_t)argv[0];
}

uint16_t wgl_btn_get_ink_out_time(const wgl_obj_t btn)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)btn;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_GET_INK_OUT_TIME);
    return (uint16_t)argv[0];
}
//
//const wgl_style_t * wgl_btn_get_style(const wgl_obj_t btn, wgl_btn_style_t type)
//{
//    //TODO: pack style
//    //wasm_btn_get_style(btn, type);
//    return NULL;
//}
