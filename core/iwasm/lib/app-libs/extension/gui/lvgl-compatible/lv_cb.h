/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WAMR_GRAPHIC_LIBRARY_CB_LVGL_COMPATIBLE_H
#define WAMR_GRAPHIC_LIBRARY_CB_LVGL_COMPATIBLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../inc/wgl_cb.h"

/** Checkbox styles. */
enum {
    LV_CB_STYLE_BG, /**< Style of object background. */
    LV_CB_STYLE_BOX_REL, /**< Style of box (released). */
    LV_CB_STYLE_BOX_PR, /**< Style of box (pressed). */
    LV_CB_STYLE_BOX_TGL_REL, /**< Style of box (released but checked). */
    LV_CB_STYLE_BOX_TGL_PR, /**< Style of box (pressed and checked). */
    LV_CB_STYLE_BOX_INA, /**< Style of disabled box */
};
typedef wgl_cb_style_t lv_cb_style_t;


#define lv_cb_create wgl_cb_create
#define lv_cb_set_text wgl_cb_set_text
#define lv_cb_set_static_text wgl_cb_set_static_text
#define lv_cb_get_text(cb) wgl_cb_get_text(cb, g_widget_text, 100)

#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_CB_LVGL_COMPATIBLE_H */
