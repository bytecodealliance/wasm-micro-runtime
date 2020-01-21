/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "native_interface.h"
#include "lvgl.h"
#include "module_wasm_app.h"
#include "wgl_native_utils.h"
#include "bh_assert.h"

/* -------------------------------------------------------------------------
 * List widget native function wrappers
 * -------------------------------------------------------------------------*/
static int32
lv_list_create_wrapper(wasm_module_inst_t module_inst,
                       lv_obj_t *par, lv_obj_t *copy)
{
    return wgl_native_wigdet_create(WIDGET_TYPE_LIST, par, copy, module_inst);
}

static int32
lv_list_add_btn_wrapper(wasm_module_inst_t module_inst,
                        lv_obj_t *list, const char *text)
{
    uint32 btn_obj_id;
    lv_obj_t *btn;
    uint32 mod_id;

    btn = lv_list_add_btn(list, NULL, text);

    if (btn == NULL)
        return 0;

    mod_id = app_manager_get_module_id(Module_WASM_App, module_inst);
    bh_assert(mod_id != ID_NONE);

    if (wgl_native_add_object(btn, mod_id, &btn_obj_id))
        return btn_obj_id; /* success return */

    return 0;
}

static WGLNativeFuncDef list_native_func_defs[] = {
    { LIST_FUNC_ID_CREATE, lv_list_create_wrapper, HAS_RET, 3, {1 | NULL_OK, 2 | NULL_OK, -1},  {-1} },
    { LIST_FUNC_ID_ADD_BTN, lv_list_add_btn_wrapper, HAS_RET, 3, {1, -1}, {2, -1} },
};

/*************** Native Interface to Wasm App ***********/
void
wasm_list_native_call(wasm_exec_env_t exec_env,
                      int32 func_id, uint32 argv_offset, uint32 argc)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    uint32 size = sizeof(list_native_func_defs) / sizeof(WGLNativeFuncDef);

    wgl_native_func_call(module_inst,
                         list_native_func_defs,
                         size,
                         func_id,
                         argv_offset,
                         argc);
}
