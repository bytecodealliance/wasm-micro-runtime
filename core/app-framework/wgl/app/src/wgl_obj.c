/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */


#include "wa-inc/lvgl/lvgl.h"
#include "gui_api.h"
#include <stdlib.h>
#include <string.h>

#define ARGC sizeof(argv)/sizeof(uint32)
#define CALL_OBJ_NATIVE_FUNC(id) wasm_obj_native_call(id, argv, ARGC)

typedef struct _obj_evt_cb {
    struct _obj_evt_cb *next;

    lv_obj_t * obj;
    lv_event_cb_t event_cb;
} obj_evt_cb_t;

static obj_evt_cb_t *g_obj_evt_cb_list = NULL;

/* For lvgl compatible */
char g_widget_text[100];

lv_res_t lv_obj_del(lv_obj_t * obj)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)obj;
    CALL_OBJ_NATIVE_FUNC(OBJ_FUNC_ID_DEL);
    return (lv_res_t)argv[0];
}

void lv_obj_del_async(struct _lv_obj_t *obj)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)obj;
    CALL_OBJ_NATIVE_FUNC(OBJ_FUNC_ID_DEL_ASYNC);
}

void lv_obj_clean(lv_obj_t * obj)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)obj;
    CALL_OBJ_NATIVE_FUNC(OBJ_FUNC_ID_CLEAN);
}

void lv_obj_align(lv_obj_t * obj, const lv_obj_t * base, lv_align_t align, lv_coord_t x_mod, lv_coord_t y_mod)
{
    uint32 argv[5] = {0};
    argv[0] = (uint32)obj;
    argv[1] = (uint32)base;
    argv[2] = align;
    argv[3] = x_mod;
    argv[4] = y_mod;
    CALL_OBJ_NATIVE_FUNC(OBJ_FUNC_ID_ALIGN);
}

lv_event_cb_t lv_obj_get_event_cb(const lv_obj_t * obj)
{
    obj_evt_cb_t *obj_evt_cb = g_obj_evt_cb_list;
    while (obj_evt_cb != NULL) {
        if (obj_evt_cb->obj == obj) {
            return obj_evt_cb->event_cb;
        }
        obj_evt_cb = obj_evt_cb->next;
    }

    return NULL;
}

void lv_obj_set_event_cb(lv_obj_t * obj, lv_event_cb_t event_cb)
{
    obj_evt_cb_t *obj_evt_cb;
    uint32 argv[1] = {0};

    obj_evt_cb = g_obj_evt_cb_list;
    while (obj_evt_cb) {
        if (obj_evt_cb->obj == obj) {
            obj_evt_cb->event_cb = event_cb;
            return;
        }
    }

    obj_evt_cb = (obj_evt_cb_t *)malloc(sizeof(*obj_evt_cb));
    if (obj_evt_cb == NULL)
        return;

    memset(obj_evt_cb, 0, sizeof(*obj_evt_cb));
    obj_evt_cb->obj = obj;
    obj_evt_cb->event_cb = event_cb;

    if (g_obj_evt_cb_list != NULL) {
        obj_evt_cb->next = g_obj_evt_cb_list;
        g_obj_evt_cb_list = obj_evt_cb;
    } else {
        g_obj_evt_cb_list = obj_evt_cb;
    }

    argv[0] = (uint32)obj;
    CALL_OBJ_NATIVE_FUNC(OBJ_FUNC_ID_SET_EVT_CB);
}

void on_widget_event(lv_obj_t * obj, lv_event_t event)
{
    obj_evt_cb_t *obj_evt_cb = g_obj_evt_cb_list;

    while (obj_evt_cb != NULL) {
        if (obj_evt_cb->obj == obj) {
            obj_evt_cb->event_cb(obj, event);
            return;
        }
        obj_evt_cb = obj_evt_cb->next;
    }
}
