/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "lvgl.h"
#include "app_manager_export.h"
#include "module_wasm_app.h"
#include "bh_list.h"
#include "bh_thread.h"
#include "wgl_native_utils.h"


typedef struct {
    bh_list_link l;

    /* The object id. */
    uint32 obj_id;

    /* The lv object */
    lv_obj_t *obj;

    /* Module id that the obj belongs to */
    uint32 module_id;
} object_node_t;

typedef struct {
    int32 obj_id;
    lv_event_t event;
} object_event_t;

/* Max obj id */
static uint32 g_obj_id_max = 0;

static bh_list g_object_list;

static korp_mutex g_object_list_mutex;

static void app_mgr_object_event_callback(module_data *m_data, bh_message_t msg)
{
    uint32 argv[2];
    wasm_function_inst_t func_on_object_event;
    bh_assert(WIDGET_EVENT_WASM == bh_message_type(msg));
    wasm_data *wasm_app_data = (wasm_data*) m_data->internal_data;
    wasm_module_inst_t inst = wasm_app_data->wasm_module_inst;
    object_event_t *object_event
                       = (object_event_t *)bh_message_payload(msg);

    if (object_event == NULL)
        return;

    func_on_object_event = wasm_runtime_lookup_function(inst, "_on_widget_event",
                                                        "(i32i32)");
    if (!func_on_object_event)
        func_on_object_event = wasm_runtime_lookup_function(inst, "on_widget_event",
                                                            "(i32i32)");
    if (!func_on_object_event) {
        printf("Cannot find function _on_object_event\n");
        return;
    }

    argv[0] = object_event->obj_id;
    argv[1] = object_event->event;
    if (!wasm_runtime_call_wasm(inst, NULL, func_on_object_event, 2, argv)) {
        printf(":Got exception running wasm code: %s\n",
                wasm_runtime_get_exception(inst));
        wasm_runtime_clear_exception(inst);
        return;
    }
}

static void cleanup_object_list(uint32 module_id)
{
    object_node_t *elem;

    vm_mutex_lock(&g_object_list_mutex);

    while (true) {
        bool found = false;
        elem = (object_node_t *)bh_list_first_elem(&g_object_list);
        while (elem) {
            /* delete the leaf node belongs to the module firstly */
            if (module_id == elem->module_id &&
                lv_obj_count_children(elem->obj) == 0) {
                object_node_t *next = (object_node_t *)bh_list_elem_next(elem);

                found = true;
                lv_obj_del(elem->obj);
                bh_list_remove(&g_object_list, elem);
                bh_free(elem);
                elem = next;
            } else {
                elem = (object_node_t *)bh_list_elem_next(elem);
            }
        }

        if (!found)
            break;
    }

    vm_mutex_unlock(&g_object_list_mutex);
}

static bool init_object_event_callback_framework()
{
    if (!wasm_register_cleanup_callback(cleanup_object_list)) {
        goto fail;
    }

    if (!wasm_register_msg_callback(WIDGET_EVENT_WASM,
                                    app_mgr_object_event_callback)) {
        goto fail;
    }

    return true;

fail:
    return false;
}

bool wgl_native_validate_object(int32 obj_id, lv_obj_t **obj)
{
    object_node_t *elem;

    vm_mutex_lock(&g_object_list_mutex);

    elem = (object_node_t *)bh_list_first_elem(&g_object_list);
    while (elem) {
        if (obj_id == elem->obj_id) {
            if (obj != NULL)
                *obj = elem->obj;
            vm_mutex_unlock(&g_object_list_mutex);
            return true;
        }
        elem = (object_node_t *) bh_list_elem_next(elem);
    }

    vm_mutex_unlock(&g_object_list_mutex);

    return false;
}

bool wgl_native_add_object(lv_obj_t *obj, uint32 module_id, uint32 *obj_id)
{
    object_node_t *node;

    node = (object_node_t *) bh_malloc(sizeof(object_node_t));

    if (node == NULL)
        return false;

    /* Generate an obj id */
    g_obj_id_max++;
    if (g_obj_id_max == -1)
        g_obj_id_max = 1;

    memset(node, 0, sizeof(*node));
    node->obj = obj;
    node->obj_id = g_obj_id_max;
    node->module_id = module_id;

    vm_mutex_lock(&g_object_list_mutex);
    bh_list_insert(&g_object_list, node);
    vm_mutex_unlock(&g_object_list_mutex);

    if (obj_id != NULL)
        *obj_id = node->obj_id;

    return true;
}

static void _obj_del_recursive(lv_obj_t *obj)
{
    object_node_t *elem;
    lv_obj_t * i;
    lv_obj_t * i_next;

    i = lv_ll_get_head(&(obj->child_ll));

    while (i != NULL) {
        /*Get the next object before delete this*/
        i_next = lv_ll_get_next(&(obj->child_ll), i);

        /*Call the recursive del to the child too*/
        _obj_del_recursive(i);

        /*Set i to the next node*/
        i = i_next;
    }

    vm_mutex_lock(&g_object_list_mutex);

    elem = (object_node_t *)bh_list_first_elem(&g_object_list);
    while (elem) {
        if (obj == elem->obj) {
            bh_list_remove(&g_object_list, elem);
            bh_free(elem);
            vm_mutex_unlock(&g_object_list_mutex);
            return;
        }
        elem = (object_node_t *) bh_list_elem_next(elem);
    }

    vm_mutex_unlock(&g_object_list_mutex);
}

static void _obj_clean_recursive(lv_obj_t *obj)
{
    lv_obj_t * i;
    lv_obj_t * i_next;

    i = lv_ll_get_head(&(obj->child_ll));

    while (i != NULL) {
        /*Get the next object before delete this*/
        i_next = lv_ll_get_next(&(obj->child_ll), i);

        /*Call the recursive del to the child too*/
        _obj_del_recursive(i);

        /*Set i to the next node*/
        i = i_next;
    }
}

static void post_widget_msg_to_module(object_node_t *object_node, lv_event_t event)
{
    module_data *module = module_data_list_lookup_id(object_node->module_id);
    object_event_t *object_event;

    if (module == NULL)
        return;

    object_event = (object_event_t *)bh_malloc(sizeof(*object_event));
    if (object_event == NULL)
        return;

    memset(object_event, 0, sizeof(*object_event));
    object_event->obj_id = object_node->obj_id;
    object_event->event = event;

    bh_post_msg(module->queue,
                WIDGET_EVENT_WASM,
                object_event,
                sizeof(*object_event));
}

static void internal_lv_obj_event_cb(lv_obj_t *obj, lv_event_t event)
{
    object_node_t *elem;

    vm_mutex_lock(&g_object_list_mutex);

    elem = (object_node_t *)bh_list_first_elem(&g_object_list);
    while (elem) {
        if (obj == elem->obj) {
            post_widget_msg_to_module(elem, event);
            vm_mutex_unlock(&g_object_list_mutex);
            return;
        }
        elem = (object_node_t *) bh_list_elem_next(elem);
    }

    vm_mutex_unlock(&g_object_list_mutex);
}

static void* lv_task_handler_thread_routine (void *arg)
{
    korp_sem sem;

    vm_sem_init(&sem, 0);

    while (true) {
        vm_sem_reltimedwait(&sem, 100);
        lv_task_handler();
    }

    return NULL;
}

void wgl_init(void)
{
    korp_thread tid;

    lv_init();

    bh_list_init(&g_object_list);
    vm_recursive_mutex_init(&g_object_list_mutex);
    init_object_event_callback_framework();

    /* new a thread, call lv_task_handler periodically */
    vm_thread_create(&tid,
                     lv_task_handler_thread_routine,
                     NULL,
                     BH_APPLET_PRESERVED_STACK_SIZE);
}

/* -------------------------------------------------------------------------
 * Obj native function wrappers
 * -------------------------------------------------------------------------*/
static lv_res_t
lv_obj_del_wrapper(wasm_module_inst_t module_inst, lv_obj_t *obj)
{
    (void)module_inst;

    /* Recursively delete object node in the list belong to this
     * parent object including itself */
    _obj_del_recursive(obj);

    return lv_obj_del(obj);
}

static void
lv_obj_del_async_wrapper(wasm_module_inst_t module_inst, lv_obj_t * obj)
{
    (void)module_inst;
    lv_obj_del_async(obj);
}

static void
lv_obj_clean_wrapper(wasm_module_inst_t module_inst, lv_obj_t *obj)
{
    (void)module_inst;

    /* Recursively delete child object node in the list belong to this
     * parent object */
    _obj_clean_recursive(obj);

    /* Delete all of its children */
    lv_obj_clean(obj);
}

static void
lv_obj_align_wrapper(wasm_module_inst_t module_inst,
                     lv_obj_t * obj,
                     const lv_obj_t * base,
                     lv_align_t align,
                     lv_coord_t x_mod,
                     lv_coord_t y_mod)
{
    (void)module_inst;
    lv_obj_align(obj, base, align, x_mod, y_mod);
}

static void
lv_obj_set_event_cb_wrapper(wasm_module_inst_t module_inst, lv_obj_t *obj)
{
    (void)module_inst;
    lv_obj_set_event_cb(obj, internal_lv_obj_event_cb);
}
/* ------------------------------------------------------------------------- */


static WGLNativeFuncDef obj_native_func_defs[] = {
    { OBJ_FUNC_ID_DEL, lv_obj_del_wrapper, HAS_RET, 2, {1, -1}, {-1} },
    { OBJ_FUNC_ID_DEL_ASYNC, lv_obj_del_async_wrapper, NO_RET, 2, {1, -1}, {-1} },
    { OBJ_FUNC_ID_CLEAN, lv_obj_clean_wrapper, NO_RET, 2, {1, -1}, {-1} },
    { OBJ_FUNC_ID_ALIGN, lv_obj_align_wrapper, NO_RET, 6, {1, 2 | NULL_OK, -1}, {-1} },
    { OBJ_FUNC_ID_SET_EVT_CB, lv_obj_set_event_cb_wrapper, NO_RET, 2, {1, -1}, {-1} },
};

/*************** Native Interface to Wasm App ***********/
void
wasm_obj_native_call(wasm_module_inst_t module_inst,
                     int32 func_id, uint32 argv_offset, uint32 argc)
{
    uint32 size = sizeof(obj_native_func_defs) / sizeof(WGLNativeFuncDef);

    wgl_native_func_call(module_inst,
                         obj_native_func_defs,
                         size,
                         func_id,
                         argv_offset,
                         argc);
}
