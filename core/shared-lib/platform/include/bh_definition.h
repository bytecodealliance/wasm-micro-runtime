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

#ifndef _BH_DEFINITION_H
#define _BH_DEFINITION_H

#include "bh_config.h"

typedef enum {
    BH_FAILED = -100,
    BH_UNKOWN = -99,
    BH_MAGIC_UNMATCH = -12,
    BH_UNIMPLEMENTED = -11,
    BH_INTR = -10,
    BH_CLOSED = -9,
    BH_BUFFER_OVERFLOW = -8, /* TODO: no used error, should remove*/
    BH_NOT_SUPPORTED = -7,
    BH_WEAR_OUT_VIOLATION = -6,
    BH_NOT_FOUND = -5,
    BH_INVALID_PARAMS = -4,
    BH_ACCESS_DENIED = -3,
    BH_OUT_OF_MEMORY = -2,
    BH_INVALID = -1,
    BH_SUCCESS = 0,
    BH_TIMEOUT = 2
} bh_status;

#endif
