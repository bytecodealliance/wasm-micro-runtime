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

#include "attr_container.h"

#ifdef __cplusplus
extern "C" {
#endif

/* board producer define sensor */
struct _sensor;
typedef struct _sensor *sensor_t;

/**
 * @typedef sensor_event_handler_f
 *
 * @brief Define the signature of callback function for API
 * sensor_open() to handle sensor event.
 *
 * @param sensor the sensor which the event belong to
 * @param sensor_event the sensor event
 * @param user_data user data associated with the sensor which is set when
 * calling sensor_open().
 *
 * @see sensor_open
 */
typedef void (*sensor_event_handler_f)(sensor_t sensor,
                                     attr_container_t *sensor_event,
                                     void *user_data);

/*
 *****************
 * Sensor APIs
 *****************
 */

/**
 * @brief Open sensor.
 *
 * @param name sensor name
 * @param index sensor index
 * @param handler callback function to handle the sensor event
 * @param user_data user data
 *
 * @return the sensor opened if success, NULL otherwise
 */
sensor_t sensor_open(const char* name,
                     int index,
                     sensor_event_handler_f handler,
                     void *user_data);

/**
 * @brief Configure sensor with interval/bit_cfg/delay values.
 *
 * @param sensor the sensor to be configured
 * @param interval sensor event interval
 * @param bit_cfg sensor bit config
 * @param delay sensor delay
 *
 * @return true if success, false otherwise
 */
bool sensor_config(sensor_t sensor, int interval, int bit_cfg, int delay);

/**
 * @brief Configure sensor with attr_container_t object.
 *
 * @param sensor the sensor to be configured
 * @param cfg the configuration
 *
 * @return true if success, false otherwise
 */
bool sensor_config_with_attr_container(sensor_t sensor, attr_container_t *cfg);

/**
 * @brief Close sensor.
 *
 * @param sensor the sensor to be closed
 *
 * @return true if success, false otherwise
 */
bool sensor_close(sensor_t sensor);

#ifdef __cplusplus
}
#endif

#endif
