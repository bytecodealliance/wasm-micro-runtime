

#ifndef WAMR_GRAPHIC_LIBRARY_NATIVE_UTILS_H
#define WAMR_GRAPHIC_LIBRARY_NATIVE_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bh_platform.h"
#include "lvgl.h"

#define OBJ_ARG_NUM_MAX 4
#define PTR_ARG_NUM_MAX 4

#define NULL_OK  0x80

enum {
    HAS_RET,
    NO_RET
};

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

    /* whether has return value */
    uint8 has_ret;

    /* argument number */
    uint8 arg_num;

    /* low 7 bit: obj argument index
     * highest 1 bit: allow obj be null or not
     * -1 means the end of this array */
    uint8 obj_arg_indexes[OBJ_ARG_NUM_MAX];

    /* pointer argument indexes, -1 means the end of this array */
    uint8 ptr_arg_indexes[PTR_ARG_NUM_MAX];
} WGLNativeFuncDef;

bool wgl_native_validate_object(int32 obj_id, lv_obj_t **obj);

bool wgl_native_add_object(lv_obj_t *obj, uint32 module_id, uint32 *obj_id);

uint32 wgl_native_wigdet_create(int8 widget_type,
                                lv_obj_t *par,
                                lv_obj_t *copy);

void wgl_native_func_call(WGLNativeFuncDef *funcs,
                          uint32 size,
                          int32 func_id,
                          uint32 argv_offset,
                          uint32 argc);

#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_NATIVE_UTILS_H */
