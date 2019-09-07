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

#ifndef WAMR_GRAPHIC_LIBRARY_LABEL_LVGL_COMPATIBLE_H
#define WAMR_GRAPHIC_LIBRARY_LABEL_LVGL_COMPATIBLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../inc/wgl_label.h"

/** Long mode behaviors. Used in 'lv_label_ext_t' */
enum {
    LV_LABEL_LONG_EXPAND,    /**< Expand the object size to the text size*/
    LV_LABEL_LONG_BREAK,     /**< Keep the object width, break the too long lines and expand the object
                                height*/
    LV_LABEL_LONG_DOT,       /**< Keep the size and write dots at the end if the text is too long*/
    LV_LABEL_LONG_SROLL,      /**< Keep the size and roll the text back and forth*/
    LV_LABEL_LONG_SROLL_CIRC, /**< Keep the size and roll the text circularly*/
    LV_LABEL_LONG_CROP,      /**< Keep the size and crop the text out of it*/
};
typedef wgl_label_long_mode_t lv_label_long_mode_t;

/** Label align policy*/
enum {
    LV_LABEL_ALIGN_LEFT, /**< Align text to left */
    LV_LABEL_ALIGN_CENTER, /**< Align text to center */
    LV_LABEL_ALIGN_RIGHT, /**< Align text to right */
};
typedef wgl_label_align_t lv_label_align_t;

/** Label styles*/
enum {
    LV_LABEL_STYLE_MAIN,
};
typedef wgl_label_style_t lv_label_style_t;


#define lv_label_create wgl_label_create
#define lv_label_set_text wgl_label_set_text
#define lv_label_get_text(label) wgl_label_get_text(label, g_widget_text, 100)

#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_LABEL_LVGL_COMPATIBLE_H */
