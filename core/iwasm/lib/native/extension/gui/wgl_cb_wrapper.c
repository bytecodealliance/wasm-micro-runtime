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

#include "lvgl.h"
#include "wasm_export.h"
#include "native_interface.h"
#include "module_wasm_app.h"
#include "wgl_native_utils.h"

/* -------------------------------------------------------------------------
 * Label widget native function wrappers
 * -------------------------------------------------------------------------*/
static int32
lv_cb_create_wrapper(wasm_module_inst_t module_inst,
                     lv_obj_t *par, lv_obj_t *copy)
{
    return wgl_native_wigdet_create(WIDGET_TYPE_CB, par, copy, module_inst);
}

static void
lv_cb_set_text_wrapper(wasm_module_inst_t module_inst,
                       lv_obj_t * cb, const char * txt)
{
    (void)module_inst;
    lv_cb_set_text(cb, txt);
}

static void
lv_cb_set_static_text_wrapper(wasm_module_inst_t module_inst,
                              lv_obj_t * cb, const char * txt)
{
    (void)module_inst;
    lv_cb_set_static_text(cb, txt);
}

static int32
lv_cb_get_text_length_wrapper(wasm_module_inst_t module_inst,
                              lv_obj_t *cb)
{
    const char *text = lv_cb_get_text(cb);

    if (text == NULL)
        return 0;

    return strlen(text);
}

static char *
lv_cb_get_text_wrapper(wasm_module_inst_t module_inst,
                       lv_obj_t *cb, char *buffer, int buffer_len)
{
    const char *text = lv_cb_get_text(cb);

    if (text == NULL)
        return 0;

    strncpy(buffer, text, buffer_len - 1);
    buffer[buffer_len - 1] = '\0';

    return buffer;
}

static WGLNativeFuncDef cb_native_func_defs[] = {
        { CB_FUNC_ID_CREATE, lv_cb_create_wrapper, HAS_RET, 3, {1 | NULL_OK, 2 | NULL_OK, -1}, {-1} },
        { CB_FUNC_ID_SET_TEXT, lv_cb_set_text_wrapper, NO_RET, 3, {1, -1}, {2, -1} },
        { CB_FUNC_ID_SET_STATIC_TEXT, lv_cb_set_static_text_wrapper, NO_RET, 3, {1, -1}, {2, -1} },
        { CB_FUNC_ID_GET_TEXT_LENGTH, lv_cb_get_text_length_wrapper, HAS_RET, 2, {1, -1}, {-1} },
        { CB_FUNC_ID_GET_TEXT, lv_cb_get_text_wrapper, RET_PTR, 4, {1, -1}, {2, -1} },
};

/*************** Native Interface to Wasm App ***********/
void
wasm_cb_native_call(wasm_module_inst_t module_inst,
                    int32 func_id, uint32 argv_offset, uint32 argc)
{
    uint32 size = sizeof(cb_native_func_defs) / sizeof(WGLNativeFuncDef);

    wgl_native_func_call(module_inst,
                         cb_native_func_defs,
                         size,
                         func_id,
                         argv_offset,
                         argc);
}
