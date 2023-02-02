/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include "wasi_nn.h"

#include <fcntl.h>
#include <errno.h>

#define MAX_MODEL_SIZE 85000000
#define MAX_OUTPUT_TENSOR_SIZE 200
#define INPUT_TENSOR_DIMS 4
#define EPSILON 1e-8

typedef struct {
    float *input_tensor;
    uint32_t *dim;
    uint32_t elements;
} input_info;

// WASI-NN wrappers

error
wasm_load(char *model_name, graph *g, execution_target target)
{
    FILE *pFile = fopen(model_name, "r");
    if (pFile == NULL)
        return invalid_argument;

    uint8_t *buffer;
    size_t result;

    // allocate memory to contain the whole file:
    buffer = (uint8_t *)malloc(sizeof(uint8_t) * MAX_MODEL_SIZE);
    if (buffer == NULL) {
        fclose(pFile);
        return missing_memory;
    }

    result = fread(buffer, 1, MAX_MODEL_SIZE, pFile);
    if (result <= 0) {
        fclose(pFile);
        free(buffer);
        return missing_memory;
    }

    graph_builder_array arr;

    arr.size = 1;
    arr.buf = (graph_builder *)malloc(sizeof(graph_builder));
    if (arr.buf == NULL) {
        fclose(pFile);
        free(buffer);
        return missing_memory;
    }

    arr.buf[0].size = result;
    arr.buf[0].buf = buffer;

    error res = load(&arr, tensorflowlite, target, g);

    fclose(pFile);
    free(buffer);
    free(arr.buf);
    return res;
}

error
wasm_init_execution_context(graph g, graph_execution_context *ctx)
{
    return init_execution_context(g, ctx);
}

error
wasm_set_input(graph_execution_context ctx, float *input_tensor, uint32_t *dim)
{
    tensor_dimensions dims;
    dims.size = INPUT_TENSOR_DIMS;
    dims.buf = (uint32_t *)malloc(dims.size * sizeof(uint32_t));
    if (dims.buf == NULL)
        return missing_memory;

    tensor tensor;
    tensor.dimensions = &dims;
    for (int i = 0; i < tensor.dimensions->size; ++i)
        tensor.dimensions->buf[i] = dim[i];
    tensor.type = fp32;
    tensor.data = (uint8_t *)input_tensor;
    error err = set_input(ctx, 0, &tensor);

    free(dims.buf);
    return err;
}

error
wasm_compute(graph_execution_context ctx)
{
    return compute(ctx);
}

error
wasm_get_output(graph_execution_context ctx, uint32_t index, float *out_tensor,
                uint32_t *out_size)
{
    return get_output(ctx, index, (uint8_t *)out_tensor, out_size);
}

// Inference

float *
run_inference(execution_target target, float *input, uint32_t *input_size,
              uint32_t *output_size, char *model_name,
              uint32_t num_output_tensors)
{
    graph graph;
    if (wasm_load(model_name, &graph, target) != success) {
        fprintf(stderr, "Error when loading model.");
        exit(1);
    }

    graph_execution_context ctx;
    if (wasm_init_execution_context(graph, &ctx) != success) {
        fprintf(stderr, "Error when initialixing execution context.");
        exit(1);
    }

    if (wasm_set_input(ctx, input, input_size) != success) {
        fprintf(stderr, "Error when setting input tensor.");
        exit(1);
    }

    if (wasm_compute(ctx) != success) {
        fprintf(stderr, "Error when running inference.");
        exit(1);
    }

    float *out_tensor = (float *)malloc(sizeof(float) * MAX_OUTPUT_TENSOR_SIZE);
    if (out_tensor == NULL) {
        fprintf(stderr, "Error when allocating memory for output tensor.");
        exit(1);
    }

    uint32_t offset = 0;
    for (int i = 0; i < num_output_tensors; ++i) {
        *output_size = MAX_OUTPUT_TENSOR_SIZE - *output_size;
        if (wasm_get_output(ctx, i, &out_tensor[offset], output_size)
            != success) {
            fprintf(stderr, "Error when getting output .");
            exit(1);
        }

        offset += *output_size;
    }
    *output_size = offset;
    return out_tensor;
}

// UTILS

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

// TESTS

void
test_sum(execution_target target)
{
    int dims[] = { 1, 5, 5, 1 };
    input_info input = create_input(dims);

    uint32_t output_size = 0;
    float *output = run_inference(target, input.input_tensor, input.dim,
                                  &output_size, "/assets/models/sum.tflite", 1);

    assert(output_size == 1);
    assert(fabs(output[0] - 300.0) < EPSILON);

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

void
test_max(execution_target target)
{
    int dims[] = { 1, 5, 5, 1 };
    input_info input = create_input(dims);

    uint32_t output_size = 0;
    float *output = run_inference(target, input.input_tensor, input.dim,
                                  &output_size, "/assets/models/max.tflite", 1);

    assert(output_size == 1);
    assert(fabs(output[0] - 24.0) < EPSILON);
    printf("Result: max is %f\n", output[0]);

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

void
test_average(execution_target target)
{
    int dims[] = { 1, 5, 5, 1 };
    input_info input = create_input(dims);

    uint32_t output_size = 0;
    float *output =
        run_inference(target, input.input_tensor, input.dim, &output_size,
                      "/assets/models/average.tflite", 1);

    assert(output_size == 1);
    assert(fabs(output[0] - 12.0) < EPSILON);
    printf("Result: average is %f\n", output[0]);

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

void
test_mult_dimensions(execution_target target)
{
    int dims[] = { 1, 3, 3, 1 };
    input_info input = create_input(dims);

    uint32_t output_size = 0;
    float *output =
        run_inference(target, input.input_tensor, input.dim, &output_size,
                      "/assets/models/mult_dim.tflite", 1);

    assert(output_size == 9);
    for (int i = 0; i < 9; i++)
        assert(fabs(output[i] - i) < EPSILON);

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

void
test_mult_outputs(execution_target target)
{
    int dims[] = { 1, 4, 4, 1 };
    input_info input = create_input(dims);

    uint32_t output_size = 0;
    float *output =
        run_inference(target, input.input_tensor, input.dim, &output_size,
                      "/assets/models/mult_out.tflite", 2);

    assert(output_size == 8);
    // first tensor check
    for (int i = 0; i < 4; i++)
        assert(fabs(output[i] - (i * 4 + 24)) < EPSILON);
    // second tensor check
    for (int i = 0; i < 4; i++)
        assert(fabs(output[i + 4] - (i + 6)) < EPSILON);

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

int
main()
{
    char *env = getenv("TARGET");
    if (env == NULL) {
        printf("Usage:\n--env=\"TARGET=[cpu|gpu]\"\n");
        return 1;
    }
    execution_target target;
    if (strcmp(env, "cpu") == 0)
        target = cpu;
    else if (strcmp(env, "gpu") == 0)
        target = gpu;
    else {
        printf("Wrong target!");
        return 1;
    }
    printf("################### Testing sum...\n");
    test_sum(target);
    printf("################### Testing max...\n");
    test_max(target);
    printf("################### Testing average...\n");
    test_average(target);
    printf("################### Testing multiple dimensions...\n");
    test_mult_dimensions(target);
    printf("################### Testing multiple outputs...\n");
    test_mult_outputs(target);

    printf("Tests: passed!\n");
    return 0;
}
