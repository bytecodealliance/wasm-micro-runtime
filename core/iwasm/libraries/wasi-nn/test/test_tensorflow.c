/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "utils.h"
#include "logger.h"

void
test_sum()
{
    int dims[] = { 1, 5, 5, 1 };
    input_info input = create_input(dims);

    uint32_t output_size = 0;
    float *output =
        run_inference(input.input_tensor, input.dim, &output_size, "sum", 1);

    if (output) {
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
        assert((output_size / sizeof(float)) == 1);
#elif WASM_ENABLE_WASI_NN != 0
        assert(output_size == 1);
#endif
        assert(fabs(output[0] - 300.0) < EPSILON);
    }

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

void
test_max()
{
    int dims[] = { 1, 5, 5, 1 };
    input_info input = create_input(dims);

    uint32_t output_size = 0;
    float *output =
        run_inference(input.input_tensor, input.dim, &output_size, "max", 1);

    if (output) {
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
        assert((output_size / sizeof(float)) == 1);
#elif WASM_ENABLE_WASI_NN != 0
        assert(output_size == 1);
#endif
        assert(fabs(output[0] - 24.0) < EPSILON);
        NN_INFO_PRINTF("Result: max is %f", output[0]);
    }

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

void
test_average()
{
    int dims[] = { 1, 5, 5, 1 };
    input_info input = create_input(dims);

    uint32_t output_size = 0;
    float *output = run_inference(input.input_tensor, input.dim, &output_size,
                                  "average", 1);

    if (output) {
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
        assert((output_size / sizeof(float)) == 1);
#elif WASM_ENABLE_WASI_NN != 0
        assert(output_size == 1);
#endif
        assert(fabs(output[0] - 12.0) < EPSILON);
        NN_INFO_PRINTF("Result: average is %f", output[0]);
    }

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

void
test_mult_dimensions()
{
    int dims[] = { 1, 3, 3, 1 };
    input_info input = create_input(dims);

    uint32_t output_size = 0;
    float *output = run_inference(input.input_tensor, input.dim, &output_size,
                                  "mult_dim", 1);

    if (output) {
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
        assert((output_size / sizeof(float)) == 9);
#elif WASM_ENABLE_WASI_NN != 0
        assert(output_size == 9);
#endif
        for (int i = 0; i < 9; i++)
            assert(fabs(output[i] - i) < EPSILON);
    }

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

void
test_mult_outputs()
{
    int dims[] = { 1, 4, 4, 1 };
    input_info input = create_input(dims);

    uint32_t output_size = 0;
    float *output = run_inference(input.input_tensor, input.dim, &output_size,
                                  "mult_out", 2);

    if (output) {
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
        assert((output_size / sizeof(float)) == 8);
#elif WASM_ENABLE_WASI_NN != 0
        assert(output_size == 8);
#endif
        // first tensor check
        for (int i = 0; i < 4; i++)
            assert(fabs(output[i] - (i * 4 + 24)) < EPSILON);
        // second tensor check
        for (int i = 0; i < 4; i++)
            assert(fabs(output[i + 4] - (i + 6)) < EPSILON);
    }

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

int
main()
{
    NN_INFO_PRINTF("Usage:\niwasm --native-lib=./libwasi_nn_tflite.so "
                   "--wasi-nn-graph=modelA:encodingA:targetA:<modelA_path> "
                   "--wasi-nn-graph=modelB:encodingB:targetB:<modelB_path>..."
                   " test_tensorflow.wasm");

    NN_INFO_PRINTF("################### Testing sum...");
    test_sum();
    NN_INFO_PRINTF("################### Testing max...");
    test_max();
    NN_INFO_PRINTF("################### Testing average...");
    test_average();
    NN_INFO_PRINTF("################### Testing multiple dimensions...");
    test_mult_dimensions();
    NN_INFO_PRINTF("################### Testing multiple outputs...");
    test_mult_outputs();

    NN_INFO_PRINTF("Tests: passed!");
    return 0;
}
