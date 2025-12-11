/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "utils.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>

WASI_NN_ERROR_TYPE
wasm_load(char *model_name, WASI_NN_NAME(graph) * g,
          WASI_NN_NAME(execution_target) target)
{
    FILE *pFile = fopen(model_name, "r");
    if (pFile == NULL)
        return WASI_NN_ERROR_NAME(invalid_argument);

    uint8_t *buffer;
    size_t result;

    // allocate memory to contain the whole file:
    buffer = (uint8_t *)malloc(sizeof(uint8_t) * MAX_MODEL_SIZE);
    if (buffer == NULL) {
        fclose(pFile);
        return WASI_NN_ERROR_NAME(too_large);
    }

    result = fread(buffer, 1, MAX_MODEL_SIZE, pFile);
    if (result <= 0) {
        fclose(pFile);
        free(buffer);
        return WASI_NN_ERROR_NAME(too_large);
    }

#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
    WASI_NN_NAME(graph_builder) arr;

    arr.buf = buffer;
    arr.size = result;

    WASI_NN_ERROR_TYPE res = WASI_NN_NAME(load)(
        &arr, result, WASI_NN_ENCODING_NAME(tensorflowlite), target, g);
#else
    WASI_NN_NAME(graph_builder_array) arr;

    arr.size = 1;
    arr.buf = (WASI_NN_NAME(graph_builder) *)malloc(
        sizeof(WASI_NN_NAME(graph_builder)));
    if (arr.buf == NULL) {
        fclose(pFile);
        free(buffer);
        return too_large;
    }

    arr.buf[0].size = result;
    arr.buf[0].buf = buffer;

    WASI_NN_ERROR_TYPE res = WASI_NN_NAME(load)(
        &arr, WASI_NN_ENCODING_NAME(tensorflowlite), target, g);
#endif

    fclose(pFile);
    free(buffer);
    free(arr.buf);
    return res;
}

WASI_NN_ERROR_TYPE
wasm_load_by_name(const char *model_name, WASI_NN_NAME(graph) * g)
{
    WASI_NN_ERROR_TYPE res =
        WASI_NN_NAME(load_by_name)(model_name, strlen(model_name), g);
    return res;
}

WASI_NN_ERROR_TYPE
wasm_init_execution_context(WASI_NN_NAME(graph) g,
                            WASI_NN_NAME(graph_execution_context) * ctx)
{
    return WASI_NN_NAME(init_execution_context)(g, ctx);
}

WASI_NN_ERROR_TYPE
wasm_set_input(WASI_NN_NAME(graph_execution_context) ctx, float *input_tensor,
               uint32_t *dim)
{
    WASI_NN_NAME(tensor_dimensions) dims;
    dims.size = INPUT_TENSOR_DIMS;
    dims.buf = (uint32_t *)malloc(dims.size * sizeof(uint32_t));
    if (dims.buf == NULL)
        return WASI_NN_ERROR_NAME(too_large);

    WASI_NN_NAME(tensor) tensor;
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
    tensor.dimensions = dims;
    for (int i = 0; i < tensor.dimensions.size; ++i)
        tensor.dimensions.buf[i] = dim[i];
    tensor.type = WASI_NN_TYPE_NAME(fp32);
    tensor.data.buf = (uint8_t *)input_tensor;

    uint32_t tmp_size = 1;
    if (dim)
        for (int i = 0; i < INPUT_TENSOR_DIMS; ++i)
            tmp_size *= dim[i];

    tensor.data.size = (tmp_size * sizeof(float));
#else
    tensor.dimensions = &dims;
    for (int i = 0; i < tensor.dimensions->size; ++i)
        tensor.dimensions->buf[i] = dim[i];
    tensor.type = WASI_NN_TYPE_NAME(fp32);
    tensor.data = (uint8_t *)input_tensor;
#endif

    WASI_NN_ERROR_TYPE err = WASI_NN_NAME(set_input)(ctx, 0, &tensor);

    free(dims.buf);
    return err;
}

WASI_NN_ERROR_TYPE
wasm_compute(WASI_NN_NAME(graph_execution_context) ctx)
{
    return WASI_NN_NAME(compute)(ctx);
}

WASI_NN_ERROR_TYPE
wasm_get_output(WASI_NN_NAME(graph_execution_context) ctx, uint32_t index,
                float *out_tensor, uint32_t *out_size)
{
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
    return WASI_NN_NAME(get_output)(ctx, index, (uint8_t *)out_tensor,
                                    MAX_OUTPUT_TENSOR_SIZE, out_size);
#else
    return WASI_NN_NAME(get_output)(ctx, index, (uint8_t *)out_tensor,
                                    out_size);
#endif
}

float *
run_inference(float *input, uint32_t *input_size, uint32_t *output_size,
              char *model_name, uint32_t num_output_tensors)
{
    WASI_NN_NAME(graph) graph;

    if (wasm_load_by_name(model_name, &graph) != WASI_NN_ERROR_NAME(success)) {
        NN_ERR_PRINTF("Error when loading model.");
        exit(1);
    }

    WASI_NN_NAME(graph_execution_context) ctx;
    if (wasm_init_execution_context(graph, &ctx)
        != WASI_NN_ERROR_NAME(success)) {
        NN_ERR_PRINTF("Error when initialixing execution context.");
        exit(1);
    }

    if (wasm_set_input(ctx, input, input_size) != WASI_NN_ERROR_NAME(success)) {
        NN_ERR_PRINTF("Error when setting input tensor.");
        exit(1);
    }

    if (wasm_compute(ctx) != WASI_NN_ERROR_NAME(success)) {
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
            != WASI_NN_ERROR_NAME(success)) {
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
