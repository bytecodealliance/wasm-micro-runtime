/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wasi_nn.h"
#include "utils.h"

#define NN_ERR_PRINTF(...)                                                    \
    do {                                                                       \
        printf("Error: ");                                                     \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
    } while (0)

#define NN_WARN_PRINTF(...)                                                   \
    do {                                                                       \
        printf("Warning: ");                                                   \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
    } while (0)

#define NN_INFO_PRINTF(...)                                                   \
    do {                                                                       \
        printf("Info: ");                                                      \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
    } while (0)

wasi_nn_error
wasm_load(char *model_name, graph *g, execution_target target)
{
    FILE *pFile = fopen(model_name, "rb");
    if (pFile == NULL) {
        NN_ERR_PRINTF("Error opening file %s", model_name);
        return invalid_argument;
    }

    fseek(pFile, 0, SEEK_END);
    long lSize = ftell(pFile);
    rewind(pFile);

    if (lSize > MAX_MODEL_SIZE) {
        NN_ERR_PRINTF("Model size too large: %ld", lSize);
        fclose(pFile);
        return too_large;
    }

    uint8_t *buffer = (uint8_t *)malloc(lSize);
    if (buffer == NULL) {
        NN_ERR_PRINTF("Memory allocation error");
        fclose(pFile);
        return runtime_error;
    }

    size_t result = fread(buffer, 1, lSize, pFile);
    if (result != lSize) {
        NN_ERR_PRINTF("Error reading file");
        fclose(pFile);
        free(buffer);
        return runtime_error;
    }

    graph_builder builder;
    builder.buf = buffer;
    builder.size = result;

    graph_builder_array arr;
    arr.size = 1;
    arr.buf = &builder;

    graph_encoding encoding = tensorflowlite;
    const char *ext = strrchr(model_name, '.');
    if (ext && strcmp(ext, ".onnx") == 0) {
        encoding = onnx;
    } else if (ext && strcmp(ext, ".tflite") == 0) {
        encoding = tensorflowlite;
    }

    wasi_nn_error res = load(&arr, encoding, target, g);

    fclose(pFile);
    free(buffer);

    return res;
}

wasi_nn_error
wasm_init_execution_context(graph g, graph_execution_context *ctx)
{
    return init_execution_context(g, ctx);
}

wasi_nn_error
wasm_set_input(graph_execution_context ctx, float *input_tensor, uint32_t *dim)
{
    tensor_dimensions dims;
    dims.size = INPUT_TENSOR_DIMS;
    dims.buf = dim;

    tensor input;
    input.dimensions = &dims;
    input.type = fp32;
    input.data = (uint8_t *)input_tensor;

    return set_input(ctx, 0, &input);
}

wasi_nn_error
wasm_compute(graph_execution_context ctx)
{
    return compute(ctx);
}

wasi_nn_error
wasm_get_output(graph_execution_context ctx, uint32_t index, float *out_tensor,
                uint32_t *out_size)
{
    return get_output(ctx, index, (uint8_t *)out_tensor, out_size);
}

float *
run_inference(execution_target target, float *input, uint32_t *input_size,
              uint32_t *output_size, char *model_name,
              uint32_t num_output_tensors)
{
    graph g;
    wasi_nn_error err = wasm_load(model_name, &g, target);
    if (err != success) {
        NN_ERR_PRINTF("Error when loading model: %d.", err);
        return NULL;
    }

    graph_execution_context ctx;
    err = wasm_init_execution_context(g, &ctx);
    if (err != success) {
        NN_ERR_PRINTF("Error when initializing execution context: %d.", err);
        return NULL;
    }

    err = wasm_set_input(ctx, input, input_size);
    if (err != success) {
        NN_ERR_PRINTF("Error when setting input tensor: %d.", err);
        return NULL;
    }

    err = wasm_compute(ctx);
    if (err != success) {
        NN_ERR_PRINTF("Error when computing: %d.", err);
        return NULL;
    }

    float *out_tensor = (float *)malloc(MAX_OUTPUT_TENSOR_SIZE);
    if (out_tensor == NULL) {
        NN_ERR_PRINTF("Memory allocation error");
        return NULL;
    }

    uint32_t offset = 0;
    for (int i = 0; i < num_output_tensors; ++i) {
        uint32_t remaining_size = MAX_OUTPUT_TENSOR_SIZE - (offset * sizeof(float));
        if (wasm_get_output(ctx, i, &out_tensor[offset], &remaining_size)
            != success) {
            NN_ERR_PRINTF("Error when getting index %d.", i);
            break;
        }

        offset += remaining_size / sizeof(float);
    }
    *output_size = offset;
    return out_tensor;
}

input_info
create_input(int *dims)
{
    input_info info;
    uint32_t elements = 1;
    for (int i = 0; i < INPUT_TENSOR_DIMS; ++i) {
        elements *= dims[i];
    }

    info.input_tensor = (float *)malloc(elements * sizeof(float));
    info.dim = (uint32_t *)malloc(INPUT_TENSOR_DIMS * sizeof(uint32_t));
    for (int i = 0; i < INPUT_TENSOR_DIMS; ++i) {
        info.dim[i] = dims[i];
    }
    info.elements = elements;

    return info;
}
