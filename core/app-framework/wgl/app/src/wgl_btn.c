/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wa-inc/lvgl/lvgl.h"
#include "bh_platform.h"
#include "gui_api.h"

#define ARGC sizeof(argv)/sizeof(uint32)
#define CALL_BTN_NATIVE_FUNC(id) wasm_btn_native_call(id, argv, ARGC)

lv_obj_t * lv_btn_create(lv_obj_t * par, const lv_obj_t * copy)
{
    uint32 argv[2] = {0};

    argv[0] = (uint32)par;
    argv[1] = (uint32)copy;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_CREATE);
    return (lv_obj_t *)argv[0];
}

void lv_btn_set_toggle(lv_obj_t * btn, bool tgl)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)btn;
    argv[1] = tgl;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_SET_TOGGLE);
}

void lv_btn_set_state(lv_obj_t * btn, lv_btn_state_t state)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)btn;
    argv[1] = state;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_SET_STATE);
}

void lv_btn_toggle(lv_obj_t * btn)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)btn;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_TOGGLE);
}

void lv_btn_set_ink_in_time(lv_obj_t * btn, uint16_t time)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)btn;
    argv[1] = time;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_SET_INK_IN_TIME);
}

void lv_btn_set_ink_wait_time(lv_obj_t * btn, uint16_t time)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)btn;
    argv[1] = time;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_SET_INK_WAIT_TIME);
}

void lv_btn_set_ink_out_time(lv_obj_t * btn, uint16_t time)
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
lv_btn_state_t lv_btn_get_state(const lv_obj_t * btn)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)btn;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_GET_STATE);
    return (lv_btn_state_t)argv[0];
}

bool lv_btn_get_toggle(const lv_obj_t * btn)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)btn;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_GET_TOGGLE);
    return (bool)argv[0];
}

uint16_t lv_btn_get_ink_in_time(const lv_obj_t * btn)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)btn;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_GET_INK_IN_TIME);
    return (uint16_t)argv[0];
}

uint16_t lv_btn_get_ink_wait_time(const lv_obj_t * btn)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)btn;
    CALL_BTN_NATIVE_FUNC(BTN_FUNC_ID_GET_INK_WAIT_TIME);
    return (uint16_t)argv[0];
}

uint16_t lv_btn_get_ink_out_time(const lv_obj_t * btn)
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
