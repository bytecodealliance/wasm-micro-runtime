/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WAMR_GRAPHIC_LIBRARY_NATIVE_UTILS_H
#define WAMR_GRAPHIC_LIBRARY_NATIVE_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bh_platform.h"
#include "lvgl.h"
#include "wasm_export.h"
#include "bi-inc/wgl_shared_utils.h"

#define wgl_native_return_type(type) type *wgl_ret = (type *)(args_ret)
#define wgl_native_get_arg(type, name) type name = *((type *)(args++))
#define wgl_native_set_return(val) *wgl_ret = (val)

#define DEFINE_WGL_NATIVE_WRAPPER(func_name)                      \
    static void func_name(wasm_exec_env_t exec_env, uint64 *args, \
                          uint32 *args_ret)

enum {
    WIDGET_TYPE_BTN,
    WIDGET_TYPE_LABEL,
    WIDGET_TYPE_CB,
    WIDGET_TYPE_LIST,
    WIDGET_TYPE_DDLIST,

    _WIDGET_TYPE_NUM,
};

typedef struct WGLNativeFuncDef {
    /* Function id */
    int32 func_id;

    /* Native function pointer */
    void *func_ptr;

    /* argument number */
    uint8 arg_num;

    /* whether the first argument is lvgl object and needs validate */
    bool check_obj;
} WGLNativeFuncDef;

bool
wgl_native_validate_object(int32 obj_id, lv_obj_t **obj);

bool
wgl_native_add_object(lv_obj_t *obj, uint32 module_id, uint32 *obj_id);

uint32
wgl_native_wigdet_create(int8 widget_type, uint32 par_obj_id,
                         uint32 copy_obj_id, wasm_module_inst_t module_inst);

void
wgl_native_func_call(wasm_exec_env_t exec_env, WGLNativeFuncDef *funcs,
                     uint32 size, int32 func_id, uint32 *argv, uint32 argc);

#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_NATIVE_UTILS_H */
