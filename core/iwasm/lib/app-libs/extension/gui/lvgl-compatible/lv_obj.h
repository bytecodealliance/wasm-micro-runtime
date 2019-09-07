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

#ifndef WAMR_GRAPHIC_LIBRARY_OBJ_LVGL_COMPATIBLE_H
#define WAMR_GRAPHIC_LIBRARY_OBJ_LVGL_COMPATIBLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../inc/wgl_obj.h"

typedef void lv_obj_t;

enum {
    LV_EVENT_PRESSED,
    LV_EVENT_PRESSING,
    LV_EVENT_PRESS_LOST,
    LV_EVENT_SHORT_CLICKED,
    LV_EVENT_LONG_PRESSED,
    LV_EVENT_LONG_PRESSED_REPEAT,
    LV_EVENT_CLICKED,
    LV_EVENT_RELEASED,
    LV_EVENT_DRAG_BEGIN,
    LV_EVENT_DRAG_END,
    LV_EVENT_DRAG_THROW_BEGIN,
    LV_EVENT_KEY,
    LV_EVENT_FOCUSED,
    LV_EVENT_DEFOCUSED,
    LV_EVENT_VALUE_CHANGED,
    LV_EVENT_INSERT,
    LV_EVENT_REFRESH,
    LV_EVENT_APPLY,
    LV_EVENT_CANCEL,
    LV_EVENT_DELETE,
};
typedef wgl_event_t lv_event_t;


/** Object alignment. */
enum {
    LV_ALIGN_CENTER,
    LV_ALIGN_IN_TOP_LEFT,
    LV_ALIGN_IN_TOP_MID,
    LV_ALIGN_IN_TOP_RIGHT,
    LV_ALIGN_IN_BOTTOM_LEFT,
    LV_ALIGN_IN_BOTTOM_MID,
    LV_ALIGN_IN_BOTTOM_RIGHT,
    LV_ALIGN_IN_LEFT_MID,
    LV_ALIGN_IN_RIGHT_MID,
    LV_ALIGN_OUT_TOP_LEFT,
    LV_ALIGN_OUT_TOP_MID,
    LV_ALIGN_OUT_TOP_RIGHT,
    LV_ALIGN_OUT_BOTTOM_LEFT,
    LV_ALIGN_OUT_BOTTOM_MID,
    LV_ALIGN_OUT_BOTTOM_RIGHT,
    LV_ALIGN_OUT_LEFT_TOP,
    LV_ALIGN_OUT_LEFT_MID,
    LV_ALIGN_OUT_LEFT_BOTTOM,
    LV_ALIGN_OUT_RIGHT_TOP,
    LV_ALIGN_OUT_RIGHT_MID,
    LV_ALIGN_OUT_RIGHT_BOTTOM,
};
typedef wgl_align_t lv_align_t;


enum {
    LV_DRAG_DIR_HOR,
    LV_DRAG_DIR_VER,
    LV_DRAG_DIR_ALL,
};

typedef wgl_drag_dir_t lv_drag_dir_t;


#define lv_obj_align wgl_obj_align
#define lv_obj_set_event_cb wgl_obj_set_event_cb
#define lv_obj_del wgl_obj_del
#define lv_obj_del_async wgl_obj_del_async
#define lv_obj_clean wgl_obj_clean


#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_OBJ_LVGL_COMPATIBLE_H */
