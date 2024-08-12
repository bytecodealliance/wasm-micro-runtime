/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include "wasi_nn_types.h"
#include "llama.h"

__attribute__((visibility("default"))) wasi_nn_error
init_backend(void **ctx)
{
    llama_backend_init();
    return success;
}

__attribute__((visibility("default"))) wasi_nn_error
deinit_backend(void *ctx)
{
    llama_backend_free();
    return success;
}

__attribute__((visibility("default"))) wasi_nn_error
load(void *ctx, graph_builder_array *builder, graph_encoding encoding,
     execution_target target, graph *g)
{
    return unsupported_operation;
}

__attribute__((visibility("default"))) wasi_nn_error
load_by_name(void *ctx, const char *filename, uint32_t filename_len, graph *g)
{
    return unsupported_operation;
}

__attribute__((visibility("default"))) wasi_nn_error
init_execution_context(void *ctx, graph g, graph_execution_context *exec_ctx)
{
    return unsupported_operation;
}

__attribute__((visibility("default"))) wasi_nn_error
set_input(void *ctx, graph_execution_context exec_ctx, uint32_t index,
          tensor *wasi_nn_tensor)
{
    return unsupported_operation;
}

__attribute__((visibility("default"))) wasi_nn_error
compute(void *ctx, graph_execution_context exec_ctx)
{
    return unsupported_operation;
}

__attribute__((visibility("default"))) wasi_nn_error
get_output(void *ctx, graph_execution_context exec_ctx, uint32_t index,
           tensor_data output_tensor, uint32_t *output_tensor_size)
{
    return unsupported_operation;
}
