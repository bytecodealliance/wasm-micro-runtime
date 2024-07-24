/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WASI_NN_OPENVINO_HPP
#define WASI_NN_OPENVINO_HPP

#include "wasi_nn_types.h"

wasi_nn_error
openvino_load(void *ctx, graph_builder_array *builder, graph_encoding encoding,
              execution_target target, graph *g);

wasi_nn_error
openvino_init_execution_context(void *ctx, graph g,
                                graph_execution_context *exec_ctx);

wasi_nn_error
openvino_set_input(void *ctx, graph_execution_context exec_ctx, uint32_t index,
                   tensor *input_tensor);

wasi_nn_error
openvino_compute(void *ctx, graph_execution_context exec_ctx);

wasi_nn_error
openvino_get_output(void *ctx, graph_execution_context exec_ctx, uint32_t index,
                    tensor_data output_tensor, uint32_t *output_tensor_size);

wasi_nn_error
openvino_initialize(void **ctx);

wasi_nn_error
openvino_destroy(void *ctx);

#endif /* WASI_NN_OPENVINO_HPP */