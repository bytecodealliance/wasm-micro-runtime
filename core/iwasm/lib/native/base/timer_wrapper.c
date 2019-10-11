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

#include "runtime_timer.h"
#include "app_manager_export.h"
#include "module_wasm_app.h"
#include "bh_list.h"
#include "bh_thread.h"
#include "bh_time.h"

bh_list g_timer_ctx_list;
korp_cond g_timer_ctx_list_cond;
korp_mutex g_timer_ctx_list_mutex;
typedef struct {
    bh_list_link l;
    timer_ctx_t timer_ctx;
} timer_ctx_node_t;

void wasm_timer_callback(timer_id_t id, unsigned int mod_id)
{
    module_data* module = module_data_list_lookup_id(mod_id);
    if (module == NULL)
        return;

    // !!! the length parameter must be 0, so the receiver will
    //     not free the payload pointer.
    bh_post_msg(module->queue, TIMER_EVENT_WASM, (char *)(uintptr_t)id, 0);
}

///
/// why we create a separate link for module timer contexts
/// rather than traverse the module list?
/// It helps to reduce the lock frequency for the module list.
/// Also when we lock the module list and then call the callback for
/// timer expire, the callback is request the list lock again for lookup
/// the module from module id. It is for avoiding that situation.

void * thread_modulers_timer_check(void * arg)
{

    int ms_to_expiry;
    while (1) {
        ms_to_expiry = -1;
        vm_mutex_lock(&g_timer_ctx_list_mutex);
        timer_ctx_node_t* elem = (timer_ctx_node_t*)
                                 bh_list_first_elem(&g_timer_ctx_list);
        while (elem) {
            int next = check_app_timers(elem->timer_ctx);
            if (next != -1) {
                if (ms_to_expiry == -1 || ms_to_expiry > next)
                    ms_to_expiry = next;
            }

            elem = (timer_ctx_node_t*) bh_list_elem_next(elem);
        }
        vm_mutex_unlock(&g_timer_ctx_list_mutex);

        if (ms_to_expiry == -1)
            ms_to_expiry = 60 * 1000;
        vm_mutex_lock(&g_timer_ctx_list_mutex);
        vm_cond_reltimedwait(&g_timer_ctx_list_cond, &g_timer_ctx_list_mutex,
                             ms_to_expiry);
        vm_mutex_unlock(&g_timer_ctx_list_mutex);
    }
}

void wakeup_modules_timer_thread(timer_ctx_t ctx)
{
    vm_mutex_lock(&g_timer_ctx_list_mutex);
    vm_cond_signal(&g_timer_ctx_list_cond);
    vm_mutex_unlock(&g_timer_ctx_list_mutex);
}

void init_wasm_timer()
{
    korp_tid tm_tid;
    bh_list_init(&g_timer_ctx_list);

    vm_cond_init(&g_timer_ctx_list_cond);
    /* temp solution for: thread_modulers_timer_check thread would recursive lock the mutex */
    vm_recursive_mutex_init(&g_timer_ctx_list_mutex);

    vm_thread_create(&tm_tid, thread_modulers_timer_check,
                     NULL, BH_APPLET_PRESERVED_STACK_SIZE);
}

timer_ctx_t create_wasm_timer_ctx(unsigned int module_id, int prealloc_num)
{
    timer_ctx_t ctx = create_timer_ctx(wasm_timer_callback,
                                       wakeup_modules_timer_thread,
                                       prealloc_num,
                                       module_id);

    if (ctx == NULL)
        return NULL;

    timer_ctx_node_t * node = (timer_ctx_node_t*)
                              bh_malloc(sizeof(timer_ctx_node_t));
    if (node == NULL) {
        destroy_timer_ctx(ctx);
        return NULL;
    }
    memset(node, 0, sizeof(*node));
    node->timer_ctx = ctx;

    vm_mutex_lock(&g_timer_ctx_list_mutex);
    bh_list_insert(&g_timer_ctx_list, node);
    vm_mutex_unlock(&g_timer_ctx_list_mutex);

    return ctx;
}

void destroy_module_timer_ctx(unsigned int module_id)
{
    vm_mutex_lock(&g_timer_ctx_list_mutex);
    timer_ctx_node_t* elem = (timer_ctx_node_t*)
                             bh_list_first_elem(&g_timer_ctx_list);
    while (elem) {
        if (timer_ctx_get_owner(elem->timer_ctx) == module_id) {
            bh_list_remove(&g_timer_ctx_list, elem);
            destroy_timer_ctx(elem->timer_ctx);
            bh_free(elem);
            break;
        }

        elem = (timer_ctx_node_t*) bh_list_elem_next(elem);
    }
    vm_mutex_unlock(&g_timer_ctx_list_mutex);
}

timer_ctx_t get_wasm_timer_ctx(wasm_module_inst_t module_inst)
{
    module_data * m = app_manager_get_module_data(Module_WASM_App,
                                                  module_inst);
    if (m == NULL)
        return NULL;
    return m->timer_ctx;
}

timer_id_t
wasm_create_timer(wasm_module_inst_t module_inst,
                  int interval, bool is_period, bool auto_start)
{
    return sys_create_timer(get_wasm_timer_ctx(module_inst), interval, is_period,
                            auto_start);
}

void
wasm_timer_destroy(wasm_module_inst_t module_inst, timer_id_t timer_id)
{
    sys_timer_destroy(get_wasm_timer_ctx(module_inst), timer_id);
}

void
wasm_timer_cancel(wasm_module_inst_t module_inst, timer_id_t timer_id)
{
    sys_timer_cancel(get_wasm_timer_ctx(module_inst), timer_id);
}

void
wasm_timer_restart(wasm_module_inst_t module_inst,
                   timer_id_t timer_id, int interval)
{
    sys_timer_restart(get_wasm_timer_ctx(module_inst), timer_id, interval);
}

extern uint32 get_sys_tick_ms();

uint32
wasm_get_sys_tick_ms(wasm_module_inst_t module_inst)
{
    return (uint32) bh_get_tick_ms();
}

