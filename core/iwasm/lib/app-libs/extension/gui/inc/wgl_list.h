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

#ifndef WAMR_GRAPHIC_LIBRARY_LIST_H
#define WAMR_GRAPHIC_LIBRARY_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

/** List styles. */
enum {
    WGL_LIST_STYLE_BG, /**< List background style */
    WGL_LIST_STYLE_SCRL, /**< List scrollable area style. */
    WGL_LIST_STYLE_SB, /**< List scrollbar style. */
    WGL_LIST_STYLE_EDGE_FLASH, /**< List edge flash style. */
    WGL_LIST_STYLE_BTN_REL, /**< Same meaning as the ordinary button styles. */
    WGL_LIST_STYLE_BTN_PR,
    WGL_LIST_STYLE_BTN_TGL_REL,
    WGL_LIST_STYLE_BTN_TGL_PR,
    WGL_LIST_STYLE_BTN_INA,
};
typedef uint8_t wgl_list_style_t;


/**
 * Create a list objects
 * @param par pointer to an object, it will be the parent of the new list
 * @param copy pointer to a list object, if not NULL then the new object will be copied from it
 * @return pointer to the created list
 */
wgl_obj_t wgl_list_create(wgl_obj_t par, wgl_obj_t copy);


/*======================
 * Add/remove functions
 *=====================*/

/**
 * Add a list element to the list
 * @param list pointer to list object
 * @param img_fn file name of an image before the text (NULL if unused)
 * @param txt text of the list element (NULL if unused)
 * @return pointer to the new list element which can be customized (a button)
 */
wgl_obj_t wgl_list_add_btn(wgl_obj_t list, const void * img_src, const char * txt);

#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_LIST_H */
