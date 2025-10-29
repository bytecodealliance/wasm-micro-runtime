
/*
 * Copyright (C) 2025 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * THIS FILE IS GENERATED AUTOMATICALLY, DO NOT EDIT!
 */
#ifndef AOT_COMP_OPTION_CHECKED_H
#define AOT_COMP_OPTION_CHECKED_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "aot_comp_option.h"

typedef struct {
    int error_code; // Error code (0 for success, non-zero for errors)
    union {
        // Add other types as needed
    } value;
} Result;

static inline Result
aot_call_stack_features_init_default_checked(void *features)
{
    Result res;
    // Check for null pointer parameter: features
    if (features == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    aot_call_stack_features_init_default(features);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

#endif // AOT_COMP_OPTION_CHECKED_H
