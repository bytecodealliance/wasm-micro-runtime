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
#include "gui_api.h"

#include <string.h>

#define ARGC sizeof(argv)/sizeof(uint32)
#define CALL_LIST_NATIVE_FUNC(id) wasm_list_native_call(id, argv, ARGC)


wgl_obj_t wgl_list_create(wgl_obj_t par, const wgl_obj_t copy)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)par;
    argv[1] = (uint32)copy;
    CALL_LIST_NATIVE_FUNC(LIST_FUNC_ID_CREATE);
    return (wgl_obj_t)argv[0];
}
//
//
//void wgl_list_clean(wgl_obj_t obj)
//{
//    wasm_list_clean(obj);
//}
//

wgl_obj_t wgl_list_add_btn(wgl_obj_t list, const void * img_src, const char * txt)
{
    (void)img_src; /* doesn't support img src currently */

    uint32 argv[3] = {0};
    argv[0] = (uint32)list;
    argv[1] = (uint32)txt;
    argv[2] = strlen(txt) + 1;
    CALL_LIST_NATIVE_FUNC(LIST_FUNC_ID_ADD_BTN);
    return (wgl_obj_t)argv[0];
}
//
//
//bool wgl_list_remove(const wgl_obj_t list, uint16_t index)
//{
//    return wasm_list_remove(list, index);
//}
//
//
//void wgl_list_set_single_mode(wgl_obj_t list, bool mode)
//{
//    wasm_list_set_single_mode(list, mode);
//}
//
//#if LV_USE_GROUP
//
//
//void wgl_list_set_btn_selected(wgl_obj_t list, wgl_obj_t btn)
//{
//    wasm_list_set_btn_selected(list, btn);
//}
//#endif
//
//
//void wgl_list_set_style(wgl_obj_t list, wgl_list_style_t type, const wgl_style_t * style)
//{
//    //TODO
//}
//
//
//bool wgl_list_get_single_mode(wgl_obj_t list)
//{
//    return wasm_list_get_single_mode(list);
//}
//
//
//const char * wgl_list_get_btn_text(const wgl_obj_t btn)
//{
//    return wasm_list_get_btn_text(btn);
//}
//
//wgl_obj_t wgl_list_get_btn_label(const wgl_obj_t btn)
//{
//    return wasm_list_get_btn_label(btn);
//}
//
//
//wgl_obj_t wgl_list_get_btn_img(const wgl_obj_t btn)
//{
//    return wasm_list_get_btn_img(btn);
//}
//
//
//wgl_obj_t wgl_list_get_prev_btn(const wgl_obj_t list, wgl_obj_t prev_btn)
//{
//    return wasm_list_get_prev_btn(list, prev_btn);
//}
//
//
//wgl_obj_t wgl_list_get_next_btn(const wgl_obj_t list, wgl_obj_t prev_btn)
//{
//    return wasm_list_get_next_btn(list, prev_btn);
//}
//
//
//int32_t wgl_list_get_btn_index(const wgl_obj_t list, const wgl_obj_t btn)
//{
//    return wasm_list_get_btn_index(list, btn);
//}
//
//
//uint16_t wgl_list_get_size(const wgl_obj_t list)
//{
//    return wasm_list_get_size(list);
//}
//
//#if LV_USE_GROUP
//
//wgl_obj_t wgl_list_get_btn_selected(const wgl_obj_t list)
//{
//    return wasm_list_get_btn_selected(list);
//}
//#endif
//
//
//
//const wgl_style_t * wgl_list_get_style(const wgl_obj_t list, wgl_list_style_t type)
//{
//    //TODO
//    return NULL;
//}
//
//
//void wgl_list_up(const wgl_obj_t list)
//{
//    wasm_list_up(list);
//}
//
//void wgl_list_down(const wgl_obj_t list)
//{
//    wasm_list_down(list);
//}
//
//
//void wgl_list_focus(const wgl_obj_t btn, wgl_anim_enable_t anim)
//{
//    wasm_list_focus(btn, anim);
//}
//


