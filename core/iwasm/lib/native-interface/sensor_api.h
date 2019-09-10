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

#ifndef _SENSOR_API_H_
#define _SENSOR_API_H_

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32
wasm_sensor_open(const char* name, int instance);

bool
wasm_sensor_config(uint32 sensor, int interval, int bit_cfg, int delay);

bool
wasm_sensor_config_with_attr_container(uint32 sensor, char *buffer, int len);

bool
wasm_sensor_close(uint32 sensor);

#ifdef __cplusplus
}
#endif

#endif /* end of _SENSOR_API_H_ */

