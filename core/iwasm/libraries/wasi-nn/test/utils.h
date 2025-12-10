/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WASI_NN_UTILS
#define WASI_NN_UTILS

#include <stdint.h>

#include "wasi_ephemeral_nn.h"
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

WASI_NN_ERROR_TYPE
wasm_load(char *model_name, WASI_NN_NAME(graph) *g, WASI_NN_NAME(execution_target) target);

WASI_NN_ERROR_TYPE
wasm_init_execution_context(WASI_NN_NAME(graph) g, WASI_NN_NAME(graph_execution_context) *ctx);

WASI_NN_ERROR_TYPE
wasm_set_input(WASI_NN_NAME(graph_execution_context) ctx, float *input_tensor, uint32_t *dim);

WASI_NN_ERROR_TYPE
wasm_compute(WASI_NN_NAME(graph_execution_context) ctx);

WASI_NN_ERROR_TYPE
wasm_get_output(WASI_NN_NAME(graph_execution_context) ctx, uint32_t index, float *out_tensor,
                uint32_t *out_size);

/* Utils */

float *
run_inference(float *input, uint32_t *input_size,
              uint32_t *output_size, char *model_name,
              uint32_t num_output_tensors);

input_info
create_input(int *dims);

#endif
