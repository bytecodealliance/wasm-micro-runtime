
/*
 * Copyright (C) 2025 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * THIS FILE IS GENERATED AUTOMATICALLY, DO NOT EDIT!
 */
#ifndef LIB_EXPORT_CHECKED_H
#define LIB_EXPORT_CHECKED_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "lib_export.h"

typedef struct {
    int error_code; // Error code (0 for success, non-zero for errors)
    union {
        uint32_t uint32_t_value;
        // Add other types as needed
    } value;
} Result;

static inline Result
get_base_lib_export_apis_checked(void *p_base_lib_apis)
{
    Result res;
    // Check for null pointer parameter: p_base_lib_apis
    if (p_base_lib_apis == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    uint32_t original_result = get_base_lib_export_apis(p_base_lib_apis);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

#endif // LIB_EXPORT_CHECKED_H
