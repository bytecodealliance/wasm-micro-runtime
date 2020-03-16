/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WAMR_GRAPHIC_LIBRARY_TYPES_LVGL_COMPATIBLE_H
#define WAMR_GRAPHIC_LIBRARY_TYPES_LVGL_COMPATIBLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../inc/wgl_types.h"

/**
 * error codes.
 */
enum {
    LV_RES_INV,
    LV_RES_OK,
};
typedef wgl_res_t lv_res_t;

#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_TYPES_LVGL_COMPATIBLE_H */
