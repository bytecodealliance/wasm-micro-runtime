/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WAMR_GRAPHIC_LIBRARY_BTN_LVGL_COMPATIBLE_H
#define WAMR_GRAPHIC_LIBRARY_BTN_LVGL_COMPATIBLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../inc/wgl_btn.h"

/** Possible states of a button.
 * It can be used not only by buttons but other button-like objects too*/
enum {
    /**Released*/
    LV_BTN_STATE_REL,

    /**Pressed*/
    LV_BTN_STATE_PR,

    /**Toggled released*/
    LV_BTN_STATE_TGL_REL,

    /**Toggled pressed*/
    LV_BTN_STATE_TGL_PR,

    /**Inactive*/
    LV_BTN_STATE_INA,

    /**Number of states*/
    _LV_BTN_STATE_NUM,
};
typedef wgl_btn_state_t lv_btn_state_t;

/**Styles*/
enum {
    /** Release style */
    LV_BTN_STYLE_REL,

    /**Pressed style*/
    LV_BTN_STYLE_PR,

    /** Toggle released style*/
    LV_BTN_STYLE_TGL_REL,

    /** Toggle pressed style */
    LV_BTN_STYLE_TGL_PR,

    /** Inactive style*/
    LV_BTN_STYLE_INA,
};
typedef wgl_btn_style_t lv_btn_style_t;


#define lv_btn_create wgl_btn_create
#define lv_btn_set_toggle wgl_btn_set_toggle
#define lv_btn_set_state wgl_btn_set_state
#define lv_btn_toggle wgl_btn_toggle
#define lv_btn_set_ink_in_time wgl_btn_set_ink_in_time
#define lv_btn_set_ink_wait_time wgl_btn_set_ink_wait_time
#define lv_btn_set_ink_out_time wgl_btn_set_ink_out_time
#define lv_btn_get_state wgl_btn_get_state
#define lv_btn_get_toggle wgl_btn_get_toggle
#define lv_btn_get_ink_in_time wgl_btn_get_ink_in_time
#define lv_btn_get_ink_wait_time wgl_btn_get_ink_wait_time
#define lv_btn_get_ink_out_time wgl_btn_get_ink_out_time

#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_BTN_LVGL_COMPATIBLE_H */
