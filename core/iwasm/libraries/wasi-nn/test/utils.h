/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WASI_NN_UTILS
#define WASI_NN_UTILS

#include <stdint.h>

#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
#include "wasi_ephemeral_nn.h"
#elif WASM_ENABLE_WASI_NN != 0
#include "wasi_nn.h"
#endif
#include "wasi_nn_types.h"

#define MAX_MODEL_SIZE 85000000
#define MAX_OUTPUT_TENSOR_SIZE 1000000
#define INPUT_TENSOR_DIMS 4
#define EPSILON 1e-8

typedef struct {
    float *input_tensor;
    uint32_t *dim;
    uint32_t elements;
} input_info;

/* wasi-nn wrappers */

wasi_ephemeral_nn_error
wasm_load(char *model_name, wasi_ephemeral_nn_graph *g,
          wasi_ephemeral_nn_execution_target target);

wasi_ephemeral_nn_error
wasm_init_execution_context(wasi_ephemeral_nn_graph g,
                            wasi_ephemeral_nn_graph_execution_context *ctx);

wasi_ephemeral_nn_error
wasm_set_input(wasi_ephemeral_nn_graph_execution_context ctx,
               float *input_tensor, uint32_t *dim);

wasi_ephemeral_nn_error
wasm_compute(wasi_ephemeral_nn_graph_execution_context ctx);

wasi_ephemeral_nn_error
wasm_get_output(wasi_ephemeral_nn_graph_execution_context ctx, uint32_t index,
                float *out_tensor, uint32_t *out_size);

/* Utils */

float *
run_inference(float *input, uint32_t *input_size, uint32_t *output_size,
              char *model_name, uint32_t num_output_tensors);

input_info
create_input(int *dims);

#endif
