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

#ifndef _NATIVE_INTERFACE_H_
#define _NATIVE_INTERFACE_H_

/* Note: the bh_plaform.h is the only head file separately
         implemented by both [app] and [native] worlds */
#include "bh_platform.h"
#include "wasm_export.h"

#define validate_app_addr(offset, size) \
    wasm_runtime_validate_app_addr(module_inst, offset, size)

#define addr_app_to_native(offset) \
    wasm_runtime_addr_app_to_native(module_inst, offset)

#define addr_native_to_app(ptr) \
    wasm_runtime_addr_native_to_app(module_inst, ptr)

#define module_malloc(size) \
    wasm_runtime_module_malloc(module_inst, size)

#define module_free(offset) \
    wasm_runtime_module_free(module_inst, offset)

/*char *wa_strdup(const char *);*/

/*
 * request/response interfaces
 */

bool
wasm_response_send(wasm_module_inst_t module_inst,
                   int32 buffer_offset, int size);
void
wasm_register_resource(wasm_module_inst_t module_inst,
                       int32 url_offset);
void
wasm_post_request(wasm_module_inst_t module_inst,
                  int32 buffer_offset, int size);
void
wasm_sub_event(wasm_module_inst_t module_inst,
               int32 url_offset);

/*
 * sensor interfaces
 */

bool
wasm_sensor_config(wasm_module_inst_t module_inst,
                   uint32 sensor, int interval, int bit_cfg, int delay);
uint32
wasm_sensor_open(wasm_module_inst_t module_inst,
                 int32 name_offset, int instance);
bool
wasm_sensor_config_with_attr_container(wasm_module_inst_t module_inst,
                                       uint32 sensor,
                                       int32 buffer_offset, int len);
bool
wasm_sensor_close(wasm_module_inst_t module_inst,
                  uint32 sensor);

/*
 * timer interfaces
 */

typedef unsigned int timer_id_t;

timer_id_t
wasm_create_timer(wasm_module_inst_t module_inst,
                  int interval, bool is_period, bool auto_start);
void
wasm_timer_destroy(wasm_module_inst_t module_inst, timer_id_t timer_id);
void
wasm_timer_cancel(wasm_module_inst_t module_inst, timer_id_t timer_id);
void
wasm_timer_restart(wasm_module_inst_t module_inst,
                   timer_id_t timer_id, int interval);
uint32
wasm_get_sys_tick_ms(wasm_module_inst_t module_inst);

/*
 * connection interfaces
 */

uint32
wasm_open_connection(wasm_module_inst_t module_inst,
                     int32 name_offset, int32 args_offset, uint32 len);
void
wasm_close_connection(wasm_module_inst_t module_inst,
                      uint32 handle);
int
wasm_send_on_connection(wasm_module_inst_t module_inst,
                        uint32 handle, int32 data_offset, uint32 len);
bool
wasm_config_connection(wasm_module_inst_t module_inst,
                       uint32 handle, int32 cfg_offset, uint32 len);

/**
 * gui interfaces
 */

void
wasm_obj_native_call(wasm_module_inst_t module_inst,
                     int32 func_id, uint32 argv_offset, uint32 argc);

void
wasm_btn_native_call(wasm_module_inst_t module_inst,
                     int32 func_id, uint32 argv_offset, uint32 argc);

void
wasm_label_native_call(wasm_module_inst_t module_inst,
                       int32 func_id, uint32 argv_offset, uint32 argc);

void
wasm_cb_native_call(wasm_module_inst_t module_inst,
                    int32 func_id, uint32 argv_offset, uint32 argc);

void
wasm_list_native_call(wasm_module_inst_t module_inst,
                      int32 func_id, uint32 argv_offset, uint32 argc);

#endif /* end of _NATIVE_INTERFACE_H */

