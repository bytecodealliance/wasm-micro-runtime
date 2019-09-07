

#include "wgl_native_utils.h"
#include "lvgl.h"
#include "module_wasm_app.h"
#include "wasm_export.h"

#include <stdint.h>

#define THROW_EXC(msg) wasm_runtime_set_exception(get_module_inst(), msg);

void
wasm_runtime_set_exception(wasm_module_inst_t module, const char *exception);

uint32 wgl_native_wigdet_create(int8 widget_type, lv_obj_t *par, lv_obj_t *copy)
{
    uint32 obj_id;
    lv_obj_t *wigdet;

    //TODO: limit total widget number

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

    if (wgl_native_add_object(wigdet,
                              app_manager_get_module_id(Module_WASM_App),
                              &obj_id))
        return obj_id; /* success return */

    return 0;
}

static void invokeNative(intptr_t argv[], uint32 argc, void (*native_code)())
{
    switch(argc) {
        case 0:
            native_code();
            break;
        case 1:
            native_code(argv[0]);
            break;
        case 2:
            native_code(argv[0], argv[1]);
            break;
        case 3:
            native_code(argv[0], argv[1], argv[2]);
            break;
        case 4:
            native_code(argv[0], argv[1], argv[2], argv[3]);
            break;
        case 5:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4]);
            break;
        case 6:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
            break;
        case 7:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5],
                        argv[6]);
            break;
        case 8:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5],
                        argv[6], argv[7]);
            break;
        case 9:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5],
                        argv[6], argv[7], argv[8]);
            break;
        case 10:
            native_code(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5],
                        argv[6], argv[7], argv[8], argv[9]);
            break;

        default:
            /* FIXME: If this happen, add more cases. */
            wasm_runtime_set_exception(get_module_inst(),
                    "the argument number of native function exceeds maximum");
            return;
    }
}

typedef void (*GenericFunctionPointer)();
typedef int32 (*Int32FuncPtr)(intptr_t *, uint32, GenericFunctionPointer);
typedef void (*VoidFuncPtr)(intptr_t *, uint32, GenericFunctionPointer);

static Int32FuncPtr invokeNative_Int32 = (Int32FuncPtr)invokeNative;
static VoidFuncPtr invokeNative_Void = (VoidFuncPtr)invokeNative;

void wgl_native_func_call(WGLNativeFuncDef *funcs,
                          uint32 size,
                          int32 func_id,
                          uint32 argv_offset,
                          uint32 argc)
{
    WGLNativeFuncDef *func_def = funcs;
    WGLNativeFuncDef *func_def_end = func_def + size;
    uint32 *argv;
    wasm_module_inst_t module_inst = get_module_inst();

    if (!validate_app_addr(argv_offset, argc * sizeof(uint32)))
        return;

    argv = addr_app_to_native(argv_offset);

    while (func_def < func_def_end) {
        if (func_def->func_id == func_id) {
            int i, obj_arg_num = 0, ptr_arg_num = 0;
            intptr_t argv_copy_buf[16];
            intptr_t *argv_copy = argv_copy_buf;

            if (func_def->arg_num > 16) {
                argv_copy = (intptr_t *)bh_malloc(func_def->arg_num *
                                                  sizeof(intptr_t));
                if (argv_copy == NULL)
                    return;
            }

            /* Init argv_copy */
            for (i = 0; i < func_def->arg_num; i++)
                argv_copy[i] = (intptr_t)argv[i];

            /* Validate object arguments */
            i = 0;
            for (; i < OBJ_ARG_NUM_MAX && func_def->obj_arg_indexes[i] != 0xff;
                   i++, obj_arg_num++) {
                uint8 index = func_def->obj_arg_indexes[i];
                bool null_ok = index & NULL_OK;

                index = index & (~NULL_OK);

                /* Some API's allow to pass NULL obj, such as xxx_create() */
                if (argv[index] == 0) {
                    if (!null_ok) {
                        THROW_EXC("the object id is 0 and invalid");
                        goto fail;
                    }
                    /* Continue so that to pass null object validation */
                    continue;
                }

                if (!wgl_native_validate_object(argv[index], (lv_obj_t **)&argv_copy[index])) {
                    THROW_EXC("the object is invalid");
                    goto fail;
                }
            }

            /* Validate address arguments */
            i = 0;
            for (; i < PTR_ARG_NUM_MAX && func_def->ptr_arg_indexes[i] != 0xff;
                   i++, ptr_arg_num++) {
                uint8 index = func_def->ptr_arg_indexes[i];

                /* The index+1 arg is the data size to be validated */
                if (!validate_app_addr(argv[index], argv[index + 1]))
                    goto fail;

                /* Convert to native address before call lvgl function */
                argv_copy[index] = (intptr_t)addr_app_to_native(argv[index]);
            }

            if (func_def->has_ret == NO_RET)
                invokeNative_Void(argv_copy,
                                  func_def->arg_num,
                                  func_def->func_ptr);
            else
                argv[0] = invokeNative_Int32(argv_copy,
                                             func_def->arg_num,
                                             func_def->func_ptr);

            if (argv_copy != argv_copy_buf)
                bh_free(argv_copy);

            /* success return */
            return;

        fail:
            if (argv_copy != argv_copy_buf)
                bh_free(argv_copy);
            return;
        }

        func_def++;
    }

    THROW_EXC("the native widget function is not found!");
}

