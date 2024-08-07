/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WASI_NN_TENSORFLOWLITE_HPP
#define WASI_NN_TENSORFLOWLITE_HPP

#include "wasi_nn_types.h"

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((visibility("default"))) wasi_nn_error
load(void *tflite_ctx, graph_builder_array *builder, graph_encoding encoding,
     execution_target target, graph *g);

__attribute__((visibility("default"))) wasi_nn_error
load_by_name(void *tflite_ctx, const char *filename, uint32_t filename_len,
             graph *g);

__attribute__((visibility("default"))) wasi_nn_error
init_execution_context(void *tflite_ctx, graph g, graph_execution_context *ctx);

__attribute__((visibility("default"))) wasi_nn_error
set_input(void *tflite_ctx, graph_execution_context ctx, uint32_t index,
          tensor *input_tensor);

__attribute__((visibility("default"))) wasi_nn_error
compute(void *tflite_ctx, graph_execution_context ctx);

__attribute__((visibility("default"))) wasi_nn_error
get_output(void *tflite_ctx, graph_execution_context ctx, uint32_t index,
           tensor_data output_tensor, uint32_t *output_tensor_size);

__attribute__((visibility("default"))) wasi_nn_error
init_backend(void **tflite_ctx);

__attribute__((visibility("default"))) wasi_nn_error
deinit_backend(void *tflite_ctx);

#ifdef __cplusplus
}
#endif

#endif
