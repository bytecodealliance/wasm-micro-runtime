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

#ifndef WAMR_GRAPHIC_LIBRARY_OBJ_H
#define WAMR_GRAPHIC_LIBRARY_OBJ_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void * wgl_obj_t;

enum {
    WGL_EVENT_PRESSED,             /**< The object has been pressed*/
    WGL_EVENT_PRESSING,            /**< The object is being pressed (called continuously while pressing)*/
    WGL_EVENT_PRESS_LOST,          /**< User is still pressing but slid cursor/finger off of the object */
    WGL_EVENT_SHORT_CLICKED,       /**< User pressed object for a short period of time, then released it. Not called if dragged. */
    WGL_EVENT_LONG_PRESSED,        /**< Object has been pressed for at least `WGL_INDEV_LONG_PRESS_TIME`.  Not called if dragged.*/
    WGL_EVENT_LONG_PRESSED_REPEAT, /**< Called after `WGL_INDEV_LONG_PRESS_TIME` in every
                                       `WGL_INDEV_LONG_PRESS_REP_TIME` ms.  Not called if dragged.*/
    WGL_EVENT_CLICKED,             /**< Called on release if not dragged (regardless to long press)*/
    WGL_EVENT_RELEASED,            /**< Called in every cases when the object has been released*/
    WGL_EVENT_DRAG_BEGIN,
    WGL_EVENT_DRAG_END,
    WGL_EVENT_DRAG_THROW_BEGIN,
    WGL_EVENT_KEY,
    WGL_EVENT_FOCUSED,
    WGL_EVENT_DEFOCUSED,
    WGL_EVENT_VALUE_CHANGED,      /**< The object's value has changed (i.e. slider moved) */
    WGL_EVENT_INSERT,
    WGL_EVENT_REFRESH,
    WGL_EVENT_APPLY,  /**< "Ok", "Apply" or similar specific button has clicked*/
    WGL_EVENT_CANCEL, /**< "Close", "Cancel" or similar specific button has clicked*/
    WGL_EVENT_DELETE, /**< Object is being deleted */
};
typedef uint8_t wgl_event_t; /**< Type of event being sent to the object. */


/** Object alignment. */
enum {
    WGL_ALIGN_CENTER = 0,
    WGL_ALIGN_IN_TOP_LEFT,
    WGL_ALIGN_IN_TOP_MID,
    WGL_ALIGN_IN_TOP_RIGHT,
    WGL_ALIGN_IN_BOTTOM_LEFT,
    WGL_ALIGN_IN_BOTTOM_MID,
    WGL_ALIGN_IN_BOTTOM_RIGHT,
    WGL_ALIGN_IN_LEFT_MID,
    WGL_ALIGN_IN_RIGHT_MID,
    WGL_ALIGN_OUT_TOP_LEFT,
    WGL_ALIGN_OUT_TOP_MID,
    WGL_ALIGN_OUT_TOP_RIGHT,
    WGL_ALIGN_OUT_BOTTOM_LEFT,
    WGL_ALIGN_OUT_BOTTOM_MID,
    WGL_ALIGN_OUT_BOTTOM_RIGHT,
    WGL_ALIGN_OUT_LEFT_TOP,
    WGL_ALIGN_OUT_LEFT_MID,
    WGL_ALIGN_OUT_LEFT_BOTTOM,
    WGL_ALIGN_OUT_RIGHT_TOP,
    WGL_ALIGN_OUT_RIGHT_MID,
    WGL_ALIGN_OUT_RIGHT_BOTTOM,
};
typedef uint8_t wgl_align_t;


enum {
    WGL_DRAG_DIR_HOR = 0x1, /**< Object can be dragged horizontally. */
    WGL_DRAG_DIR_VER = 0x2, /**< Object can be dragged vertically. */
    WGL_DRAG_DIR_ALL = 0x3, /**< Object can be dragged in all directions. */
};

typedef uint8_t wgl_drag_dir_t;

typedef void (*wgl_event_cb_t)(wgl_obj_t obj, wgl_event_t event);

void wgl_obj_align(wgl_obj_t obj, wgl_obj_t base, wgl_align_t align, wgl_coord_t x_mod, wgl_coord_t y_mod);
void wgl_obj_set_event_cb(wgl_obj_t obj, wgl_event_cb_t event_cb);
wgl_res_t wgl_obj_del(wgl_obj_t obj);
void wgl_obj_del_async(wgl_obj_t obj);
void wgl_obj_clean(wgl_obj_t obj);



#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_OBJ_H */
