/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WAMR_GRAPHIC_LIBRARY_TYPES_H
#define WAMR_GRAPHIC_LIBRARY_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * WGL error codes.
 */
enum {
    WGL_RES_INV = 0, /*Typically indicates that the object is deleted (become invalid) in the action
                       function or an operation was failed*/
    WGL_RES_OK,      /*The object is valid (no deleted) after the action*/
};
typedef uint8_t wgl_res_t;

#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_TYPES_H */
