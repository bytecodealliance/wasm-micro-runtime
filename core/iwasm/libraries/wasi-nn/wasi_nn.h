/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASI_SOCKET_EXT_H_
#define _WASI_SOCKET_EXT_H_

#include <stddef.h>
#include <stdint.h>

typedef uint32_t buffer_size;

typedef enum:uint16_t {sucess, invalid_argument, missing_memory, busy } nn_erno; 

typedef uint32_t *  tensor_dimensions;

typedef enum:uint8_t {f16, f32, u8, i32};

typedef struct {

tensor_dimensions dimensions;

tensor_type type;

tensor_data data;

} tensor ;

typedef uint8_t * graph_builder;

typedef graph_builder * graph_builder_array;

typedef handle graph;

typedef enum {openvino=0, tensorflow,onnx } graph_encoding;
