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

#include "native_interface.h"
#include "lvgl.h"
#include "module_wasm_app.h"
#include "wgl_native_utils.h"

/* -------------------------------------------------------------------------
 * List widget native function wrappers
 * -------------------------------------------------------------------------*/
static int32 _list_create(lv_obj_t *par, lv_obj_t *copy)
{
    return wgl_native_wigdet_create(WIDGET_TYPE_LIST, par, copy);
}

static int32 _list_add_btn(lv_obj_t *list, const char *text)
{
    uint32 btn_obj_id;
    lv_obj_t *btn;

    btn = lv_list_add_btn(list, NULL, text);

    if (btn == NULL)
        return 0;

    if (wgl_native_add_object(btn,
                              app_manager_get_module_id(Module_WASM_App),
                              &btn_obj_id))
        return btn_obj_id; /* success return */

    return 0;
}

static WGLNativeFuncDef list_native_func_defs[] = {
    { LIST_FUNC_ID_CREATE, _list_create, HAS_RET, 2, {0 | NULL_OK, 1 | NULL_OK, -1},  {-1} },
    { LIST_FUNC_ID_ADD_BTN, _list_add_btn, HAS_RET, 2, {0, -1}, {1, -1} },
};

/*************** Native Interface to Wasm App ***********/
void wasm_list_native_call(int32 func_id, uint32 argv_offset, uint32 argc)
{
    uint32 size = sizeof(list_native_func_defs) / sizeof(WGLNativeFuncDef);

    wgl_native_func_call(list_native_func_defs,
                         size,
                         func_id,
                         argv_offset,
                         argc);
}
