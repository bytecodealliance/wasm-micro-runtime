
#include "wgl.h"
#include "native_interface.h"
#include <stdlib.h>
#include <string.h>

#define ARGC sizeof(argv)/sizeof(uint32)
#define CALL_OBJ_NATIVE_FUNC(id) wasm_obj_native_call(id, (int32)argv, ARGC)

typedef struct _obj_evt_cb {
    struct _obj_evt_cb *next;

    wgl_obj_t obj;

    wgl_event_cb_t event_cb;
} obj_evt_cb_t;

static obj_evt_cb_t *g_obj_evt_cb_list = NULL;

/* For lvgl compatible */
char g_widget_text[100];

wgl_res_t wgl_obj_del(wgl_obj_t obj)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)obj;
    CALL_OBJ_NATIVE_FUNC(OBJ_FUNC_ID_DEL);
    return (wgl_res_t)argv[0];
}

void wgl_obj_del_async(wgl_obj_t obj)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)obj;
    CALL_OBJ_NATIVE_FUNC(OBJ_FUNC_ID_DEL_ASYNC);
}

void wgl_obj_clean(wgl_obj_t obj)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)obj;
    CALL_OBJ_NATIVE_FUNC(OBJ_FUNC_ID_CLEAN);
}

//void wgl_obj_invalidate(const wgl_obj_t obj)
//{
//}
//void wgl_obj_set_parent(wgl_obj_t obj, wgl_obj_t parent)
//{
//}
//void wgl_obj_move_foreground(wgl_obj_t obj)
//{
//}
//void wgl_obj_move_background(wgl_obj_t obj)
//{
//}
//void wgl_obj_set_pos(wgl_obj_t obj, wgl_coord_t x, wgl_coord_t y)
//{
//}
//void wgl_obj_set_x(wgl_obj_t obj, wgl_coord_t x)
//{
//}
//void wgl_obj_set_y(wgl_obj_t obj, wgl_coord_t y)
//{
//}
//void wgl_obj_set_size(wgl_obj_t obj, wgl_coord_t w, wgl_coord_t h)
//{
//}
//void wgl_obj_set_width(wgl_obj_t obj, wgl_coord_t w)
//{
//}
//void wgl_obj_set_height(wgl_obj_t obj, wgl_coord_t h)
//{
//}
//
void wgl_obj_align(wgl_obj_t obj, const wgl_obj_t base, wgl_align_t align, wgl_coord_t x_mod, wgl_coord_t y_mod)
{
    uint32 argv[5] = {0};
    argv[0] = (uint32)obj;
    argv[1] = (uint32)base;
    argv[2] = align;
    argv[3] = x_mod;
    argv[4] = y_mod;
    CALL_OBJ_NATIVE_FUNC(OBJ_FUNC_ID_ALIGN);
}
//#if 0
//void wgl_obj_align_origo(wgl_obj_t obj, const wgl_obj_t base, wgl_align_t align, wgl_coord_t x_mod, wgl_coord_t y_mod)
//{
//    wasm_obj_align_origo(obj, base, align, x_mod, y_mod);
//}
//void wgl_obj_realign(wgl_obj_t obj)
//{
//    wasm_obj_realign(obj);
//}
//void wgl_obj_set_auto_realign(wgl_obj_t obj, bool en)
//{
//    wasm_obj_set_auto_realign(obj, en);
//}
//void wgl_obj_set_ext_click_area(wgl_obj_t obj, wgl_coord_t left, wgl_coord_t right, wgl_coord_t top, wgl_coord_t bottom)
//{
//    wasm_obj_set_ext_click_area(obj, left, right, top, bottom);
//}
//void wgl_obj_set_style(wgl_obj_t obj, const wgl_style_t * style)
//{
//    //TODO:
//    //wgl_obj_set_style(obj, );
//}
//void wgl_obj_refresh_style(wgl_obj_t obj)
//{
//    wasm_obj_refresh_style(obj);
//}
//
//void wgl_obj_report_style_mod(wgl_style_t * style)
//{
//    //TODO
//}
//void wgl_obj_set_hidden(wgl_obj_t obj, bool en)
//{
//    wasm_obj_set_hidden(obj, en);
//}
//
//void wgl_obj_set_click(wgl_obj_t obj, bool en)
//{
//    wasm_obj_set_click(obj, en);
//}
//void wgl_obj_set_top(wgl_obj_t obj, bool en)
//{
//    wasm_obj_set_top(obj, en);
//}
//void wgl_obj_set_drag(wgl_obj_t obj, bool en)
//{
//    wasm_obj_set_drag(obj, en);
//}
//void wgl_obj_set_drag_dir(wgl_obj_t obj, wgl_drag_dir_t drag_dir)
//{
//    wasm_obj_set_drag_dir(obj, drag_dir);
//}
//void wgl_obj_set_drag_throw(wgl_obj_t obj, bool en)
//{
//    wasm_obj_set_drag_throw(obj, en);
//}
//void wgl_obj_set_drag_parent(wgl_obj_t obj, bool en)
//{
//    wasm_obj_set_drag_parent(obj, en);
//}
//void wgl_obj_set_parent_event(wgl_obj_t obj, bool en)
//{
//    wasm_obj_set_parent_event(obj, en);
//}
//void wgl_obj_set_opa_scale_enable(wgl_obj_t obj, bool en)
//{
//    wasm_obj_set_opa_scale_enable(obj, en);
//}
//void wgl_obj_set_opa_scale(wgl_obj_t obj, wgl_opa_t opa_scale)
//{
//    wasm_obj_set_opa_scale(obj, opa_scale);
//}
//void wgl_obj_set_protect(wgl_obj_t obj, uint8_t prot)
//{
//    wasm_obj_set_protect(obj, prot);
//}
//void wgl_obj_clear_protect(wgl_obj_t obj, uint8_t prot)
//{
//    wasm_obj_clear_protect(obj, prot);
//}
//wgl_res_t wgl_event_send(wgl_obj_t obj, wgl_event_t event, const void * data)
//{
//    //TODO
//    return 0;
//}
//wgl_res_t wgl_event_send_func(wgl_event_cb_t event_xcb, wgl_obj_t obj, wgl_event_t event, const void * data)
//{
//    //TODO
//    return 0;
//}
//
//const void * wgl_event_get_data(void)
//{
//    //TODO
//    return NULL;
//}
//#if 0 //TODO
//void wgl_obj_set_signal_cb(wgl_obj_t obj, wgl_signal_cb_t signal_cb)
//{
//}
//void wgl_signal_send(wgl_obj_t obj, wgl_signal_t signal, void * param)
//{
//}
//void wgl_obj_set_design_cb(wgl_obj_t obj, wgl_design_cb_t design_cb)
//{
//}
//#endif
//
//#if 0
//void * wgl_obj_allocate_ext_attr(wgl_obj_t obj, uint16_t ext_size)
//{
//}
//#endif
//void wgl_obj_refresh_ext_draw_pad(wgl_obj_t obj)
//{
//    wasm_obj_refresh_ext_draw_pad(obj);
//}
//
//wgl_obj_t wgl_obj_get_screen(const wgl_obj_t obj)
//{
//    return wasm_obj_get_screen(obj);
//}
//wgl_obj_t wgl_obj_get_parent(const wgl_obj_t obj)
//{
//    return wasm_obj_get_parent(obj);
//}
//
//wgl_obj_t wgl_obj_get_child(const wgl_obj_t obj, const wgl_obj_t child)
//{
//    return wasm_obj_get_child(obj, child);
//}
//
//wgl_obj_t wgl_obj_get_child_back(const wgl_obj_t obj, const wgl_obj_t child)
//{
//    return wasm_obj_get_child_back(obj, child);
//}
//
//uint16_t wgl_obj_count_children(const wgl_obj_t obj)
//{
//    return wasm_obj_count_children(obj);
//}
//
//uint16_t wgl_obj_count_children_recursive(const wgl_obj_t obj)
//{
//    return wasm_obj_count_children_recursive(obj);
//}
//
//void wgl_obj_get_coords(const wgl_obj_t obj, wgl_area_t * cords_p)
//{
//    //TODO:
////    int size;
////    char *cords_buf = wgl_area_pack(cords_p, &size);
////
////    if (buf == NULL)
////        return;
////
////    wasm_obj_get_coords(obj, cords_buf, size);
////
////    if (wgl_area_unpack(cords_buf, size, cords_p) == NULL)
////        free(cords_buf);
//}
//
//void wgl_obj_get_inner_coords(const wgl_obj_t obj, wgl_area_t * coords_p)
//{
//    //TODO
//}
//
//wgl_coord_t wgl_obj_get_x(const wgl_obj_t obj)
//{
//    return wasm_obj_get_x(obj);
//}
//
//wgl_coord_t wgl_obj_get_y(const wgl_obj_t obj)
//{
//    return wasm_obj_get_y(obj);
//}
//
//wgl_coord_t wgl_obj_get_width(const wgl_obj_t obj)
//{
//    return wasm_obj_get_width(obj);
//}
//
//wgl_coord_t wgl_obj_get_height(const wgl_obj_t obj)
//{
//    return wasm_obj_get_height(obj);
//}
//
//wgl_coord_t wgl_obj_get_width_fit(wgl_obj_t obj)
//{
//    return wasm_obj_get_width_fit(obj);
//}
//
//wgl_coord_t wgl_obj_get_height_fit(wgl_obj_t obj)
//{
//    return wasm_obj_get_height_fit(obj);
//}
//
//bool wgl_obj_get_auto_realign(wgl_obj_t obj)
//{
//    return wasm_obj_get_auto_realign(obj);
//}
//
//wgl_coord_t wgl_obj_get_ext_click_pad_left(const wgl_obj_t obj)
//{
//    return wasm_obj_get_ext_click_pad_left(obj);
//}
//
//wgl_coord_t wgl_obj_get_ext_click_pad_right(const wgl_obj_t obj)
//{
//    return wasm_obj_get_ext_click_pad_right(obj);
//}
//
//wgl_coord_t wgl_obj_get_ext_click_pad_top(const wgl_obj_t obj)
//{
//    return wasm_obj_get_ext_click_pad_top(obj);
//}
//
//wgl_coord_t wgl_obj_get_ext_click_pad_bottom(const wgl_obj_t obj)
//{
//    return wasm_obj_get_ext_click_pad_bottom(obj);
//}
//
//wgl_coord_t wgl_obj_get_ext_draw_pad(const wgl_obj_t obj)
//{
//    return wasm_obj_get_ext_draw_pad(obj);
//}
//
//const wgl_style_t * wgl_obj_get_style(const wgl_obj_t obj)
//{
//    //TODO
//    return NULL;
//}
//
//bool wgl_obj_get_hidden(const wgl_obj_t obj)
//{
//    return wasm_obj_get_hidden(obj);
//}
//
//bool wgl_obj_get_click(const wgl_obj_t obj)
//{
//    return wasm_obj_get_click(obj);
//}
//
//bool wgl_obj_get_top(const wgl_obj_t obj)
//{
//    return wasm_obj_get_top(obj);
//}
//
//bool wgl_obj_get_drag(const wgl_obj_t obj)
//{
//    return wasm_obj_get_drag(obj);
//}
//
//wgl_drag_dir_t wgl_obj_get_drag_dir(const wgl_obj_t obj)
//{
//    return wasm_obj_get_drag_dir(obj);
//}
//
//bool wgl_obj_get_drag_throw(const wgl_obj_t obj)
//{
//    return wasm_obj_get_drag_throw(obj);
//}
//
//bool wgl_obj_get_drag_parent(const wgl_obj_t obj)
//{
//    return wasm_obj_get_drag_parent(obj);
//}
//
//bool wgl_obj_get_parent_event(const wgl_obj_t obj)
//{
//    return wasm_obj_get_parent_event(obj);
//}
//
//wgl_opa_t wgl_obj_get_opa_scale_enable(const wgl_obj_t obj)
//{
//    return wasm_obj_get_opa_scale_enable(obj);
//}
//
//wgl_opa_t wgl_obj_get_opa_scale(const wgl_obj_t obj)
//{
//    return wasm_obj_get_opa_scale(obj);
//}
//
//uint8_t wgl_obj_get_protect(const wgl_obj_t obj)
//{
//    return wasm_obj_get_protect(obj);
//}
//
//bool wgl_obj_is_protected(const wgl_obj_t obj, uint8_t prot)
//{
//    return wasm_obj_is_protected(obj, prot);
//}
//#endif
//TODO
//wgl_signal_cb_t wgl_obj_get_signal_cb(const wgl_obj_t obj);

//TODO
//wgl_design_cb_t wgl_obj_get_design_cb(const wgl_obj_t obj);

wgl_event_cb_t wgl_obj_get_event_cb(const wgl_obj_t obj)
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

//void wgl_obj_get_type(wgl_obj_t obj, wgl_obj_type_t * buf)
//{
//    //TODO
//}
//
//#if LV_USE_USER_DATA
//wgl_obj_user_data_t wgl_obj_get_user_data(wgl_obj_t obj)
//{
//    return wasm_obj_get_user_data(obj);
//}
//
//wgl_obj_user_data_t * wgl_obj_get_user_data_ptr(wgl_obj_t obj)
//{
//    return wasm_obj_get_user_data_ptr(obj);
//}
//
//void wgl_obj_set_user_data(wgl_obj_t obj, wgl_obj_user_data_t data)
//{
//    wasm_obj_set_user_data(obj, data);
//}
//#endif
//
//#if LV_USE_GROUP
//void * wgl_obj_get_group(const wgl_obj_t obj)
//{
//    return wasm_obj_get_group(obj);
//}
//
//bool wgl_obj_is_focused(const wgl_obj_t obj)
//{
//    return wasm_obj_is_focused(obj);
//}
//#endif

void wgl_obj_set_event_cb(wgl_obj_t obj, wgl_event_cb_t event_cb)
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

void on_widget_event(wgl_obj_t obj, wgl_event_t event)
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
