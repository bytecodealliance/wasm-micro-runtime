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

#ifndef WAMR_GRAPHIC_LIBRARY_LIST_LVGL_COMPATIBLE_H
#define WAMR_GRAPHIC_LIBRARY_LIST_LVGL_COMPATIBLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../inc/wgl_list.h"

/** List styles. */
enum {
    LV_LIST_STYLE_BG, /**< List background style */
    LV_LIST_STYLE_SCRL, /**< List scrollable area style. */
    LV_LIST_STYLE_SB, /**< List scrollbar style. */
    LV_LIST_STYLE_EDGE_FLASH, /**< List edge flash style. */
    LV_LIST_STYLE_BTN_REL, /**< Same meaning as the ordinary button styles. */
    LV_LIST_STYLE_BTN_PR,
    LV_LIST_STYLE_BTN_TGL_REL,
    LV_LIST_STYLE_BTN_TGL_PR,
    LV_LIST_STYLE_BTN_INA,
};
typedef wgl_list_style_t lv_list_style_t;


#define lv_list_create wgl_list_create
#define lv_list_add_btn wgl_list_add_btn

#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_LIST_LVGL_COMPATIBLE_H */
