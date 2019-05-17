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

#include "app_manager.h"
#include "app_manager_host.h"
#include "bh_queue.h"
#include "bh_memory.h"
#include "bh_thread.h"
#include "attr_container.h"
#include "event.h"
#include "watchdog.h"
#include "coap_ext.h"

/* Lock of the module data list */
korp_mutex module_data_list_lock;

/* Module data list */
module_data *module_data_list;

bool module_data_list_init()
{
    module_data_list = NULL;
    return !vm_mutex_init(&module_data_list_lock) ? true : false;
}

void module_data_list_destroy()
{

    vm_mutex_lock(&module_data_list_lock);
    if (module_data_list) {
        while (module_data_list) {
            module_data *p = module_data_list->next;
            bh_free(module_data_list);
            module_data_list = p;
        }
    }
    vm_mutex_unlock(&module_data_list_lock);
    vm_mutex_destroy(&module_data_list_lock);
}

static void module_data_list_add(module_data *m_data)
{
    static uint32 module_id_max = 1;
    vm_mutex_lock(&module_data_list_lock);
    // reserve some special ID
    // TODO: check the new id is not already occupied!
    if (module_id_max == 0xFFFFFFF0)
        module_id_max = 1;
    m_data->id = module_id_max++;
    if (!module_data_list) {
        module_data_list = m_data;
    } else {
        /* Set as head */
        m_data->next = module_data_list;
        module_data_list = m_data;
    }
    vm_mutex_unlock(&module_data_list_lock);
}

void module_data_list_remove(module_data *m_data)
{
    vm_mutex_lock(&module_data_list_lock);
    if (module_data_list) {
        if (module_data_list == m_data)
            module_data_list = module_data_list->next;
        else {
            /* Search and remove it */
            module_data *p = module_data_list;

            while (p && p->next != m_data)
                p = p->next;
            if (p && p->next == m_data)
                p->next = p->next->next;
        }
    }
    vm_mutex_unlock(&module_data_list_lock);
}

module_data*
module_data_list_lookup(const char *module_name)
{
    vm_mutex_lock(&module_data_list_lock);
    if (module_data_list) {
        module_data *p = module_data_list;

        while (p) {
            /* Search by module name */
            if (!strcmp(module_name, p->module_name)) {
                vm_mutex_unlock(&module_data_list_lock);
                return p;
            }
            p = p->next;
        }
    }
    vm_mutex_unlock(&module_data_list_lock);
    return NULL;
}

module_data*
module_data_list_lookup_id(unsigned int module_id)
{
    vm_mutex_lock(&module_data_list_lock);
    if (module_data_list) {
        module_data *p = module_data_list;

        while (p) {
            /* Search by module name */
            if (module_id == p->id) {
                vm_mutex_unlock(&module_data_list_lock);
                return p;
            }
            p = p->next;
        }
    }
    vm_mutex_unlock(&module_data_list_lock);
    return NULL;
}

module_data *
app_manager_get_module_data(uint32 module_type)
{
    if (g_module_interfaces[module_type]
            && g_module_interfaces[module_type]->module_get_module_data)
        return g_module_interfaces[module_type]->module_get_module_data();
    return NULL;
}

void*
app_manager_get_module_queue(uint32 module_type)
{
    return app_manager_get_module_data(module_type)->queue;
}

const char*
app_manager_get_module_name(uint32 module_type)
{
    return app_manager_get_module_data(module_type)->module_name;
}

unsigned int app_manager_get_module_id(uint32 module_type)
{
    return app_manager_get_module_data(module_type)->id;
}

void*
app_manager_get_module_heap(uint32 module_type)
{
    return app_manager_get_module_data(module_type)->heap;
}

module_data*
app_manager_lookup_module_data(const char *name)
{
    return module_data_list_lookup(name);
}

void app_manager_add_module_data(module_data *m_data)
{
    module_data_list_add(m_data);
}

void app_manager_del_module_data(module_data *m_data)
{
    module_data_list_remove(m_data);

    release_module(m_data);
}

bool app_manager_is_interrupting_module(uint32 module_type)
{
    return app_manager_get_module_data(module_type)->wd_timer.is_interrupting;
}

extern void destory_module_timer_ctx(unsigned int module_id);

void release_module(module_data *m_data)
{
    watchdog_timer_destroy(&m_data->wd_timer);

#ifdef HEAP_ENABLED /* TODO */
    if(m_data->heap) gc_destroy_for_instance(m_data->heap);
#endif

    if (m_data->queue)
        bh_queue_destroy(m_data->queue);

    m_data->timer_ctx = NULL;

    destory_module_timer_ctx(m_data->id);

    bh_free(m_data);
}

int check_modules_timer_expiry()
{
    vm_mutex_lock(&module_data_list_lock);
    module_data *p = module_data_list;
    int ms_to_expiry = -1;

    while (p) {

        int next = get_expiry_ms(p->timer_ctx);
        if (next != -1) {
            if (ms_to_expiry == -1 || ms_to_expiry > next)
                ms_to_expiry = next;
        }

        p = p->next;
    }
    vm_mutex_unlock(&module_data_list_lock);
    return ms_to_expiry;
}

