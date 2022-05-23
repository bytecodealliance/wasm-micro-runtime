

#include "wgl_native_utils.h"
#include "lvgl.h"
#include "module_wasm_app.h"
#include "wasm_export.h"
#include "bh_assert.h"

#include <stdint.h>

#define THROW_EXC(msg) wasm_runtime_set_exception(module_inst, msg);

uint32
wgl_native_wigdet_create(int8 widget_type, uint32 par_obj_id,
                         uint32 copy_obj_id, wasm_module_inst_t module_inst)
{
    uint32 obj_id;
    lv_obj_t *wigdet = NULL, *par = NULL, *copy = NULL;
    uint32 mod_id;

    // TODO: limit total widget number

    /* validate the parent object id if not equal to 0 */
    if (par_obj_id != 0 && !wgl_native_validate_object(par_obj_id, &par)) {
        THROW_EXC("create widget with invalid parent object.");
        return 0;
    }
    /* validate the copy object id if not equal to 0 */
    if (copy_obj_id != 0 && !wgl_native_validate_object(copy_obj_id, &copy)) {
        THROW_EXC("create widget with invalid copy object.");
        return 0;
    }

    if (par == NULL)
        par = lv_disp_get_scr_act(NULL);

    if (widget_type == WIDGET_TYPE_BTN)
        wigdet = lv_btn_create(par, copy);
    else if (widget_type == WIDGET_TYPE_LABEL)
        wigdet = lv_label_create(par, copy);
    else if (widget_type == WIDGET_TYPE_CB)
        wigdet = lv_cb_create(par, copy);
    else if (widget_type == WIDGET_TYPE_LIST)
        wigdet = lv_list_create(par, copy);
    else if (widget_type == WIDGET_TYPE_DDLIST)
        wigdet = lv_ddlist_create(par, copy);

    if (wigdet == NULL)
        return 0;

    mod_id = app_manager_get_module_id(Module_WASM_App, module_inst);
    bh_assert(mod_id != ID_NONE);

    if (wgl_native_add_object(wigdet, mod_id, &obj_id))
        return obj_id; /* success return */

    return 0;
}

void
wgl_native_func_call(wasm_exec_env_t exec_env, WGLNativeFuncDef *funcs,
                     uint32 size, int32 func_id, uint32 *argv, uint32 argc)
{
    typedef void (*WGLNativeFuncPtr)(wasm_exec_env_t, uint64 *, uint32 *);
    WGLNativeFuncPtr wglNativeFuncPtr;
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    WGLNativeFuncDef *func_def = funcs;
    WGLNativeFuncDef *func_def_end = func_def + size;

    /* Note: argv is validated in wasm_runtime_invoke_native()
     * with pointer length equals to 1. Here validate the argv
     * buffer again but with its total length in bytes */
    if (!wasm_runtime_validate_native_addr(module_inst, argv,
                                           argc * sizeof(uint32)))
        return;

    while (func_def < func_def_end) {
        if (func_def->func_id == func_id && (uint32)func_def->arg_num == argc) {
            uint64 argv_copy_buf[16], size;
            uint64 *argv_copy = argv_copy_buf;
            int i;

            if (argc > sizeof(argv_copy_buf) / sizeof(uint64)) {
                size = sizeof(uint64) * (uint64)argc;
                if (size >= UINT32_MAX
                    || !(argv_copy = wasm_runtime_malloc((uint32)size))) {
                    THROW_EXC("allocate memory failed.");
                    return;
                }
                memset(argv_copy, 0, (uint32)size);
            }

            /* Init argv_copy */
            for (i = 0; i < func_def->arg_num; i++)
                *(uint32 *)&argv_copy[i] = argv[i];

            /* Validate the first argument which is a lvgl object if needed */
            if (func_def->check_obj) {
                lv_obj_t *obj = NULL;
                if (!wgl_native_validate_object(argv[0], &obj)) {
                    THROW_EXC("the object is invalid");
                    goto fail;
                }
                *(lv_obj_t **)&argv_copy[0] = obj;
            }

            wglNativeFuncPtr = (WGLNativeFuncPtr)func_def->func_ptr;
            wglNativeFuncPtr(exec_env, argv_copy, argv);

            if (argv_copy != argv_copy_buf)
                wasm_runtime_free(argv_copy);

            /* success return */
            return;

        fail:
            if (argv_copy != argv_copy_buf)
                wasm_runtime_free(argv_copy);
            return;
        }

        func_def++;
    }

    THROW_EXC("the native widget function is not found!");
}
