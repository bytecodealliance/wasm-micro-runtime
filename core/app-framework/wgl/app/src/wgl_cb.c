/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */


#include "wa-inc/lvgl/lvgl.h"
#include "gui_api.h"

#include <string.h>

#define ARGC sizeof(argv)/sizeof(uint32)
#define CALL_CB_NATIVE_FUNC(id) wasm_cb_native_call(id, argv, ARGC)

lv_obj_t * lv_cb_create(lv_obj_t * par, const lv_obj_t * copy)
{
    uint32 argv[2] = {0};

    argv[0] = (uint32)par;
    argv[1] = (uint32)copy;
    CALL_CB_NATIVE_FUNC(CB_FUNC_ID_CREATE);
    return (lv_obj_t *)argv[0];
}

void lv_cb_set_text(lv_obj_t * cb, const char * txt)
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)cb;
    argv[1] = (uint32)txt;
    argv[2] = strlen(txt) + 1;
    CALL_CB_NATIVE_FUNC(CB_FUNC_ID_SET_TEXT);
}

void lv_cb_set_static_text(lv_obj_t * cb, const char * txt)
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)cb;
    argv[1] = (uint32)txt;
    argv[2] = strlen(txt) + 1;
    CALL_CB_NATIVE_FUNC(CB_FUNC_ID_SET_STATIC_TEXT);
}

//void wgl_cb_set_style(wgl_obj_t cb, wgl_cb_style_t type, const wgl_style_t * style)
//{
//    //TODO:
//}
//

static unsigned int wgl_cb_get_text_length(lv_obj_t * cb)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)cb;
    CALL_CB_NATIVE_FUNC(CB_FUNC_ID_GET_TEXT_LENGTH);
    return argv[0];
}

static char *wgl_cb_get_text(lv_obj_t * cb, char *buffer, int buffer_len)
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)cb;
    argv[1] = (uint32)buffer;
    argv[2] = buffer_len;
    CALL_CB_NATIVE_FUNC(CB_FUNC_ID_GET_TEXT);
    return (char *)argv[0];
}

// TODO: need to use a global data buffer for the returned text
const char * lv_cb_get_text(const lv_obj_t * cb)
{

    return NULL;
}


//const wgl_style_t * wgl_cb_get_style(const wgl_obj_t cb, wgl_cb_style_t type)
//{
//    //TODO
//    return NULL;
//}
//


