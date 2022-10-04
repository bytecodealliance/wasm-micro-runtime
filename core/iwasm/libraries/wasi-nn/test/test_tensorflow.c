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

#define BUFFER_SIZE 32

#define INPUT_SIZE 25

#define EPSILON 1e-8
#define MAX_OUTPUT_TENSOR_SIZE 200
#define MAX_MODEL_SIZE 85000000
#define MAX_OUTPUT_TENSORS 4

#define GRAPH_EXECUTION_CONTEXT 44

typedef struct {
    float *input_tensor;
    uint32_t *dim;
    uint32_t elements;
} input_info;

error
wasm_load(char *model_name, graph *graph)
{
    FILE *pFile = fopen(model_name, "r");

    assert(pFile != NULL);

    uint8_t *buffer;
    size_t result;

    // allocate memory to contain the whole file:
    buffer = (uint8_t *)malloc(sizeof(uint8_t) * MAX_MODEL_SIZE);

    if (buffer == NULL) {
        fputs("Memory error\n", stderr);
        exit(2);
    }

    result = fread(buffer, 1, MAX_MODEL_SIZE, pFile);

    graph_builder_array arr;

    arr.buf = (graph_builder *)malloc(sizeof(graph_builder));
    arr.size = 1;

    arr.buf[0].buf = buffer;
    arr.buf[0].size = result;

    error err = load(&arr, tensorflow, cpu, graph);

    fclose(pFile);
    free(buffer);
    return err;
}

error
wasm_init_execution_context(graph graph)
{
    graph_execution_context gec;
    return init_execution_context(graph, &gec);
}

error
wasm_input(float *input_tensor, uint32_t *dim)
{
    tensor_dimensions dims;
    dims.size = 4;
    dims.buf = (uint32_t *)malloc(dims.size * sizeof(uint32_t));

    tensor tensor;
    tensor.dimensions = &dims;
    for (int i = 0; i < tensor.dimensions->size; ++i)
        tensor.dimensions->buf[i] = dim[i];
    tensor.type = fp32;
    tensor.data = (uint8_t *)input_tensor;
    error err = set_input(44, 0, &tensor);

    free(tensor.dimensions->buf);
    return err;
}

error
wasm_compute(graph_execution_context context)
{
    return compute(context);
}

error
wasm_get_output(graph_execution_context context, uint32_t index,
                float *out_tensor, uint32_t *out_size)
{
    return get_output(context, index, (uint8_t *)out_tensor, out_size);
}

float *
run_inference(float *input, uint32_t *input_size, uint32_t *output_size,
              char *model_name)
{
    graph graph = 444;
    wasm_load(model_name, &graph);

    wasm_init_execution_context(graph);

    wasm_input(input, input_size);

    graph_execution_context context;
    wasm_compute(context);

    float *out_tensor = (float *)malloc(sizeof(float) * MAX_OUTPUT_TENSOR_SIZE);

    uint32_t offset = 0;
    for (int i = 0; i < MAX_OUTPUT_TENSORS; ++i) {
        *output_size = MAX_OUTPUT_TENSOR_SIZE - *output_size;
        error err =
            wasm_get_output(context, i, &out_tensor[offset], output_size);
        if (err != success)
            break;
        offset += *output_size;
    }
    *output_size = offset;
    return out_tensor;
}

// UTILS

input_info
create_input(int N, int *dims)
{
    input_info input = { .dim = NULL, .input_tensor = NULL, .elements = 1 };

    input.dim = malloc(N * sizeof(uint32_t));
    for (int i = 0; i < N; ++i) {
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
test_sum()
{
    int N = 4;
    int dims[] = { 1, 5, 5, 1 };
    input_info input = create_input(4, dims);

    uint32_t output_size = 0;
    float *output = run_inference(input.input_tensor, input.dim, &output_size,
                                  "models/sum.tflite");

    assert(output_size == 1);
    assert(fabs(output[0] - 300.0) < EPSILON);

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

void
test_max()
{
    int N = 4;
    int dims[] = { 1, 5, 5, 1 };
    input_info input = create_input(4, dims);

    uint32_t output_size = 0;
    float *output = run_inference(input.input_tensor, input.dim, &output_size,
                                  "models/max.tflite");

    assert(output_size == 1);
    assert(fabs(output[0] - 24.0) < EPSILON);
    printf("Result: max is %f\n", output[0]);

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

void
test_average()
{
    int N = 4;
    int dims[] = { 1, 5, 5, 1 };
    input_info input = create_input(4, dims);

    uint32_t output_size = 0;
    float *output = run_inference(input.input_tensor, input.dim, &output_size,
                                  "models/average.tflite");

    assert(output_size == 1);
    assert(fabs(output[0] - 12.0) < EPSILON);
    printf("Result: average is %f\n", output[0]);

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

void
test_mult_dimensions()
{
    int N = 4;
    int dims[] = { 1, 3, 3, 1 };
    input_info input = create_input(4, dims);

    uint32_t output_size = 0;
    float *output = run_inference(input.input_tensor, input.dim, &output_size,
                                  "models/mult_dim.tflite");

    assert(output_size == 9);
    for (int i = 0; i < 9; i++)
        assert(fabs(output[i] - i) < EPSILON);

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

void
test_mult_outputs()
{
    int N = 4;
    int dims[] = { 1, 4, 4, 1 };
    input_info input = create_input(4, dims);

    uint32_t output_size = 0;
    float *output = run_inference(input.input_tensor, input.dim, &output_size,
                                  "models/mult_out.tflite");

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
    printf("################### Testing sum...\n");
    test_sum();
    printf("################### Testing max...\n");
    test_max();
    printf("################### Testing average...\n");
    test_average();
    printf("################### Testing multiple dimensions...\n");
    test_mult_dimensions();
    printf("################### Testing multiple outputs...\n");
    test_mult_outputs();

    printf("Tests: passed!\n");
    return 0;
}
