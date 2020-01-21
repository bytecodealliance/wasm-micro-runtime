/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _NATIVE_INTERFACE_H_
#define _NATIVE_INTERFACE_H_

/* Note: the bh_plaform.h is the only head file separately
         implemented by both [app] and [native] worlds */
#include "bh_platform.h"
#include "wasm_export.h"

#define get_module_inst(exec_env) \
    wasm_runtime_get_module_inst(exec_env)

#define validate_app_addr(offset, size) \
    wasm_runtime_validate_app_addr(module_inst, offset, size)

#define validate_app_str_addr(offset) \
    wasm_runtime_validate_app_str_addr(module_inst, offset)

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
wasm_response_send(wasm_exec_env_t exec_env,
                   int32 buffer_offset, int size);
void
wasm_register_resource(wasm_exec_env_t exec_env,
                       int32 url_offset);
void
wasm_post_request(wasm_exec_env_t exec_env,
                  int32 buffer_offset, int size);
void
wasm_sub_event(wasm_exec_env_t exec_env,
               int32 url_offset);

/*
 * sensor interfaces
 */

bool
wasm_sensor_config(wasm_exec_env_t exec_env,
                   uint32 sensor, int interval, int bit_cfg, int delay);
uint32
wasm_sensor_open(wasm_exec_env_t exec_env,
                 int32 name_offset, int instance);
bool
wasm_sensor_config_with_attr_container(wasm_exec_env_t exec_env,
                                       uint32 sensor,
                                       int32 buffer_offset, int len);
bool
wasm_sensor_close(wasm_exec_env_t exec_env,
                  uint32 sensor);

/*
 * timer interfaces
 */

typedef unsigned int timer_id_t;

timer_id_t
wasm_create_timer(wasm_exec_env_t exec_env,
                  int interval, bool is_period, bool auto_start);
void
wasm_timer_destroy(wasm_exec_env_t exec_env, timer_id_t timer_id);
void
wasm_timer_cancel(wasm_exec_env_t exec_env, timer_id_t timer_id);
void
wasm_timer_restart(wasm_exec_env_t exec_env,
                   timer_id_t timer_id, int interval);
uint32
wasm_get_sys_tick_ms(wasm_exec_env_t exec_env);

/*
 * connection interfaces
 */

uint32
wasm_open_connection(wasm_exec_env_t exec_env,
                     int32 name_offset, int32 args_offset, uint32 len);
void
wasm_close_connection(wasm_exec_env_t exec_env,
                      uint32 handle);
int
wasm_send_on_connection(wasm_exec_env_t exec_env,
                        uint32 handle, int32 data_offset, uint32 len);
bool
wasm_config_connection(wasm_exec_env_t exec_env,
                       uint32 handle, int32 cfg_offset, uint32 len);

/**
 * gui interfaces
 */

void
wasm_obj_native_call(wasm_exec_env_t exec_env,
                     int32 func_id, uint32 argv_offset, uint32 argc);

void
wasm_btn_native_call(wasm_exec_env_t exec_env,
                     int32 func_id, uint32 argv_offset, uint32 argc);

void
wasm_label_native_call(wasm_exec_env_t exec_env,
                       int32 func_id, uint32 argv_offset, uint32 argc);

void
wasm_cb_native_call(wasm_exec_env_t exec_env,
                    int32 func_id, uint32 argv_offset, uint32 argc);

void
wasm_list_native_call(wasm_exec_env_t exec_env,
                      int32 func_id, uint32 argv_offset, uint32 argc);

#endif /* end of _NATIVE_INTERFACE_H */

