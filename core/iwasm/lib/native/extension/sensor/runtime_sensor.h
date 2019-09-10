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

#ifndef LIB_EXTENSION_RUNTIME_SENSOR_H_
#define LIB_EXTENSION_RUNTIME_SENSOR_H_

#include "bh_platform.h"
#include "attr_container.h"
#include "wasm_export.h"

struct _sys_sensor;
typedef struct _sys_sensor* sensor_obj_t;

typedef struct _sensor_client {
    struct _sensor_client * next;
    unsigned int client_id; // the app id
    int interval;
    int bit_cfg;
    int delay;
    void (*client_callback)(void * client, uint32, attr_container_t *);
} sensor_client_t;

typedef struct _sys_sensor {
    struct _sys_sensor * next;
    char * name;
    int sensor_instance;
    char * description;
    uint32 sensor_id;
    sensor_client_t * clients;
    /* app, sensor mgr and app mgr may access the clients at the same time,
     * so need a lock to protect the clients */
    korp_mutex lock;
    uint32 last_read;
    uint32 read_interval;
    uint32 default_interval;

    attr_container_t * (*read)(void *); /* TODO: may support other type return value, such as 'cbor' */
    bool (*config)(void *, void *);

} sys_sensor_t;

sensor_obj_t add_sys_sensor(char * name, char * description, int instance,
        uint32 default_interval, void * read_func, void * config_func);
sensor_obj_t find_sys_sensor(const char* name, int instance);
sensor_obj_t find_sys_sensor_id(uint32 sensor_id);
void refresh_read_interval(sensor_obj_t sensor);
void sensor_cleanup_callback(uint32 module_id);
int check_sensor_timers();
void reschedule_sensor_read();

uint32
wasm_sensor_open(wasm_module_inst_t module_inst,
                 int32 name_offset, int instance);

bool
wasm_sensor_config(wasm_module_inst_t module_inst,
                   uint32 sensor, int interval, int bit_cfg, int delay);

bool
wasm_sensor_config_with_attr_container(wasm_module_inst_t module_inst,
                                       uint32 sensor, int32 buffer_offset,
                                       int len);

bool
wasm_sensor_close(wasm_module_inst_t module_inst, uint32 sensor);

#endif /* LIB_EXTENSION_RUNTIME_SENSOR_H_ */
