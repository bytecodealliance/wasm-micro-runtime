/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WASI_NN_TENSORFLOWLITE_MICRO_HPP
#define WASI_NN_TENSORFLOWLITE_MICRO_HPP

#include "wasi_nn.h"

#ifdef __cplusplus
extern "C" {
#endif

error
tensorflowlite_micro_load(void *tflite_ctx, graph_builder_array *builder,
                    graph_encoding encoding, execution_target target, graph *g);

error
tensorflowlite_micro_init_execution_context(void *tflite_ctx, graph g,
                                      graph_execution_context *ctx);

error
tensorflowlite_micro_set_input(void *tflite_ctx, graph_execution_context ctx,
                         uint32_t index, tensor *input_tensor);

error
tensorflowlite_micro_compute(void *tflite_ctx, graph_execution_context ctx);

error
tensorflowlite_micro_get_output(void *tflite_ctx, graph_execution_context ctx,
                          uint32_t index, tensor_data output_tensor,
                          uint32_t *output_tensor_size);

void
tensorflowlite_micro_initialize(void **tflite_ctx);

void
tensorflowlite_micro_destroy(void *tflite_ctx);

#ifdef __cplusplus
}
#endif

#endif
