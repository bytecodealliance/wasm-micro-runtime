/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
