/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "utils.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>

wasi_ephemeral_nn_error
wasm_load(char *model_name, wasi_ephemeral_nn_graph *g,
          wasi_ephemeral_nn_execution_target target)
{
    FILE *pFile = fopen(model_name, "r");
    if (pFile == NULL)
        return wasi_ephemeral_nn_error_invalid_argument;

    uint8_t *buffer;
    size_t result;

    // allocate memory to contain the whole file:
    buffer = (uint8_t *)malloc(sizeof(uint8_t) * MAX_MODEL_SIZE);
    if (buffer == NULL) {
        fclose(pFile);
        return wasi_ephemeral_nn_error_too_large;
    }

    result = fread(buffer, 1, MAX_MODEL_SIZE, pFile);
    if (result <= 0) {
        fclose(pFile);
        free(buffer);
        return wasi_ephemeral_nn_error_too_large;
    }

    wasi_ephemeral_nn_graph_builder arr;

    arr.buf = buffer;
    arr.size = result;

    wasi_ephemeral_nn_error res = wasi_ephemeral_nn_load(
        &arr, result, wasi_ephemeral_nn_encoding_tensorflowlite, target, g);

    fclose(pFile);
    free(buffer);
    free(arr.buf);
    return res;
}

wasi_ephemeral_nn_error
wasm_load_by_name(const char *model_name, wasi_ephemeral_nn_graph *g)
{
    wasi_ephemeral_nn_error res =
        wasi_ephemeral_nn_load_by_name(model_name, strlen(model_name), g);
    return res;
}

wasi_ephemeral_nn_error
wasm_init_execution_context(wasi_ephemeral_nn_graph g,
                            wasi_ephemeral_nn_graph_execution_context *ctx)
{
    return wasi_ephemeral_nn_init_execution_context(g, ctx);
}

wasi_ephemeral_nn_error
wasm_set_input(wasi_ephemeral_nn_graph_execution_context ctx,
               float *input_tensor, uint32_t *dim)
{
    wasi_ephemeral_nn_tensor_dimensions dims;
    dims.size = INPUT_TENSOR_DIMS;
    dims.buf = (uint32_t *)malloc(dims.size * sizeof(uint32_t));
    if (dims.buf == NULL)
        return wasi_ephemeral_nn_error_too_large;

    wasi_ephemeral_nn_tensor tensor;
    tensor.dimensions = dims;
    for (int i = 0; i < tensor.dimensions.size; ++i)
        tensor.dimensions.buf[i] = dim[i];
    tensor.type = wasi_ephemeral_nn_type_fp32;
    tensor.data.buf = (uint8_t *)input_tensor;

    uint32_t tmp_size = 1;
    if (dim)
        for (int i = 0; i < INPUT_TENSOR_DIMS; ++i)
            tmp_size *= dim[i];

    tensor.data.size = (tmp_size * sizeof(float));

    wasi_ephemeral_nn_error err = wasi_ephemeral_nn_set_input(ctx, 0, &tensor);

    free(dims.buf);
    return err;
}

wasi_ephemeral_nn_error
wasm_compute(wasi_ephemeral_nn_graph_execution_context ctx)
{
    return wasi_ephemeral_nn_compute(ctx);
}

wasi_ephemeral_nn_error
wasm_get_output(wasi_ephemeral_nn_graph_execution_context ctx, uint32_t index,
                float *out_tensor, uint32_t *out_size)
{
    return wasi_ephemeral_nn_get_output(ctx, index, (uint8_t *)out_tensor,
                                        MAX_OUTPUT_TENSOR_SIZE, out_size);
}

float *
run_inference(float *input, uint32_t *input_size, uint32_t *output_size,
              char *model_name, uint32_t num_output_tensors)
{
    wasi_ephemeral_nn_graph graph;

    wasi_ephemeral_nn_error res = wasm_load_by_name(model_name, &graph);

    if (res == wasi_ephemeral_nn_error_not_found) {
        NN_INFO_PRINTF("Model %s is not loaded, you should pass its path "
                       "through --wasi-nn-graph",
                       model_name);
        return NULL;
    }
    else if (res != wasi_ephemeral_nn_error_success) {
        NN_ERR_PRINTF("Error when loading model.");
        exit(1);
    }

    wasi_ephemeral_nn_graph_execution_context ctx;
    if (wasm_init_execution_context(graph, &ctx)
        != wasi_ephemeral_nn_error_success) {
        NN_ERR_PRINTF("Error when initialixing execution context.");
        exit(1);
    }

    if (wasm_set_input(ctx, input, input_size)
        != wasi_ephemeral_nn_error_success) {
        NN_ERR_PRINTF("Error when setting input tensor.");
        exit(1);
    }

    if (wasm_compute(ctx) != wasi_ephemeral_nn_error_success) {
        NN_ERR_PRINTF("Error when running inference.");
        exit(1);
    }

    float *out_tensor = (float *)malloc(sizeof(float) * MAX_OUTPUT_TENSOR_SIZE);
    if (out_tensor == NULL) {
        NN_ERR_PRINTF("Error when allocating memory for output tensor.");
        exit(1);
    }

    uint32_t offset = 0;
    for (int i = 0; i < num_output_tensors; ++i) {
        *output_size = MAX_OUTPUT_TENSOR_SIZE - *output_size;
        if (wasm_get_output(ctx, i, &out_tensor[offset], output_size)
            != wasi_ephemeral_nn_error_success) {
            NN_ERR_PRINTF("Error when getting index %d.", i);
            break;
        }

        offset += *output_size;
    }
    *output_size = offset;
    return out_tensor;
}

input_info
create_input(int *dims)
{
    input_info input = { .dim = NULL, .input_tensor = NULL, .elements = 1 };

    input.dim = malloc(INPUT_TENSOR_DIMS * sizeof(uint32_t));
    if (input.dim)
        for (int i = 0; i < INPUT_TENSOR_DIMS; ++i) {
            input.dim[i] = dims[i];
            input.elements *= dims[i];
        }

    input.input_tensor = malloc(input.elements * sizeof(float));
    for (int i = 0; i < input.elements; ++i)
        input.input_tensor[i] = i;

    return input;
}
