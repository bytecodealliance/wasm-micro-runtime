/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WASI_NN_OPENVINO_HPP
#define WASI_NN_OPENVINO_HPP

#include "wasi_nn_types.h"

__attribute__((visibility("default"))) wasi_nn_error
load(void *ctx, graph_builder_array *builder, graph_encoding encoding,
     execution_target target, graph *g);

__attribute__((visibility("default"))) wasi_nn_error
init_execution_context(void *ctx, graph g, graph_execution_context *exec_ctx);

__attribute__((visibility("default"))) wasi_nn_error
set_input(void *ctx, graph_execution_context exec_ctx, uint32_t index,
          tensor *input_tensor);

__attribute__((visibility("default"))) wasi_nn_error
compute(void *ctx, graph_execution_context exec_ctx);

__attribute__((visibility("default"))) wasi_nn_error
get_output(void *ctx, graph_execution_context exec_ctx, uint32_t index,
           tensor_data output_tensor, uint32_t *output_tensor_size);

__attribute__((visibility("default"))) wasi_nn_error
init_backend(void **ctx);

__attribute__((visibility("default"))) wasi_nn_error
deinit_backend(void *ctx);

#endif /* WASI_NN_OPENVINO_HPP */