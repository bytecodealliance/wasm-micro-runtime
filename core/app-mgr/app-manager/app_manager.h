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

#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include "bh_platform.h"
#include "bh_common.h"
#include "bh_queue.h"
#include "korp_types.h"
#include "app_manager_export.h"
#include "native_interface.h"
#include "shared_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ZEPHYR__
#define app_manager_printf printf
#else
#define app_manager_printf printk
#endif

#define ID_HOST -3
#define ID_APP_MGR -2
#define ID_NONE (uint32)-1

#define SEND_ERR_RESPONSE(mid, err_msg) do {                            \
  app_manager_printf("%s\n", err_msg);                                  \
  send_error_response_to_host(mid, INTERNAL_SERVER_ERROR_5_00, err_msg);  \
} while (0)

extern module_interface *g_module_interfaces[Module_Max];

/* Lock of the module data list */
extern korp_mutex module_data_list_lock;

/* Module data list */
extern module_data *module_data_list;

void
app_manager_add_module_data(module_data *m_data);

void
app_manager_del_module_data(module_data *m_data);

bool
module_data_list_init();

void
module_data_list_destroy();

bool
app_manager_is_interrupting_module(uint32 module_type, void *module_inst);

void release_module(module_data *m_data);

void
module_data_list_remove(module_data *m_data);

void*
app_manager_timer_create(void (*timer_callback)(void*),
        watchdog_timer *wd_timer);

void
app_manager_timer_destroy(void *timer);

void
app_manager_timer_start(void *timer, int timeout);

void
app_manager_timer_stop(void *timer);

watchdog_timer*
app_manager_get_wd_timer_from_timer_handle(void *timer);

int
app_manager_signature_verify(const uint8_t *file, unsigned int file_len,
        const uint8_t *signature, unsigned int sig_size);

void targeted_app_request_handler(request_t *request, void *unused);

#if BEIHAI_ENABLE_TOOL_AGENT != 0
void *
app_manager_get_tool_agent_queue();
#endif

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif

