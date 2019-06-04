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


#include "module_wasm_lib.h"

static bool wasm_lib_module_init(void)
{
    return false;
}

static bool wasm_lib_module_install(request_t *msg)
{
    (void) msg;
    return false;
}

static bool wasm_lib_module_uninstall(request_t *msg)
{
    (void) msg;
    return false;
}

static void wasm_lib_module_watchdog_kill(module_data *m_data)
{
    (void) m_data;
}

static bool wasm_lib_module_handle_host_url(void *queue_msg)
{
    (void) queue_msg;
    return false;
}

static module_data*
wasm_lib_module_get_module_data(void)
{
    return NULL;
}

module_interface wasm_lib_module_interface = { wasm_lib_module_init,
        wasm_lib_module_install, wasm_lib_module_uninstall,
        wasm_lib_module_watchdog_kill, wasm_lib_module_handle_host_url,
        wasm_lib_module_get_module_data,
        NULL };

