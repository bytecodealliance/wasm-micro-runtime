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

#ifndef _AEE_SENSOR_H_
#define _AEE_SENSOR_H_

#include "attr-container.h"

#ifdef __cplusplus
extern "C" {
#endif

//TODO:
#define bh_queue_t void

/* board producer define sensor */
struct _sensor;
typedef struct _sensor *sensor_t;

// Sensor APIs
sensor_t sensor_open(const char* name, int index,
        void (*on_sensor_event)(sensor_t, attr_container_t *, void *),
        void *user_data);
bool sensor_config(sensor_t sensor, int interval, int bit_cfg, int delay);
bool sensor_config_with_attr_container(sensor_t sensor, attr_container_t *cfg);
bool sensor_close(sensor_t sensor);

#ifdef __cplusplus
}
#endif

#endif
