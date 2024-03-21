/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WASI_NN_APP_NATIVE
#define WASI_NN_APP_NATIVE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "wasi_nn.h"
#include "logger.h"

#include "bh_platform.h"
#include "wasm_export.h"

typedef struct {
    uint32_t buf_offset;
    uint32_t size;
} graph_builder_wasm;

typedef struct {
    uint32_t buf_offset;
    uint32_t size;
} graph_builder_array_wasm;

typedef struct {
    uint32_t buf_offset;
    uint32_t size;
} tensor_dimensions_wasm;

typedef struct {
    uint32_t dimensions_offset;
    tensor_type type;
    uint32_t data_offset;
} tensor_wasm;

#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
typedef struct {
    tensor_dimensions_wasm dimensions;
    tensor_type type;
    uint32_t data_offset;
    uint32_t data_size;
} tensor_wasm_ephemeral;
#endif /* WASM_ENABLE_WASI_EPHEMERAL_NN != 0 */

error
graph_builder_array_app_native(wasm_module_inst_t instance,
                               graph_builder_array_wasm *builder,
                               graph_builder_array *builder_native);

#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
error
graph_builder_array_app_native_ephemeral(wasm_module_inst_t instance,
                                         graph_builder_wasm *builder_wasm,
                                         uint32_t size,
                                         graph_builder_array *builder_array);
#endif /* WASM_ENABLE_WASI_EPHEMERAL_NN != 0 */

error
tensor_app_native(wasm_module_inst_t instance, tensor_wasm *input_tensor,
                  tensor *input_tensor_native);

#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
error
tensor_app_native_ephemeral(wasm_module_inst_t instance,
                            tensor_wasm_ephemeral *input_tensor_wasm,
                            tensor *input_tensor);
#endif /* WASM_ENABLE_WASI_EPHEMERAL_NN != 0 */

#endif
