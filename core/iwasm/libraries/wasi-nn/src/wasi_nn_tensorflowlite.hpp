/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WASI_NN_TENSORFLOWLITE_HPP
#define WASI_NN_TENSORFLOWLITE_HPP

#include "wasi_nn.h"

#ifdef __cplusplus
extern "C" {
#endif

error
tensorflowlite_load(graph_builder_array *builder, graph_encoding encoding,
                    execution_target target, graph *g);

error
tensorflowlite_init_execution_context(graph g, graph_execution_context *ctx);

error
tensorflowlite_set_input(graph_execution_context ctx, uint32_t index,
                         tensor *input_tensor);

error
tensorflowlite_compute(graph_execution_context ctx);

error
tensorflowlite_get_output(graph_execution_context ctx, uint32_t index,
                          tensor_data output_tensor,
                          uint32_t *output_tensor_size);

void
tensorflowlite_destroy();

#ifdef __cplusplus
}
#endif

#endif
