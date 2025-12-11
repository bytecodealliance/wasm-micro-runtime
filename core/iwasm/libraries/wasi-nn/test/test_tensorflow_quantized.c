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

#undef EPSILON
#define EPSILON 1e-2

void
test_average_quantized()
{
    int dims[] = { 1, 5, 5, 1 };
    input_info input = create_input(dims);

    uint32_t output_size = 0;
    float *output = run_inference(input.input_tensor, input.dim, &output_size,
                                  "quantized_model", 1);

    NN_INFO_PRINTF("Output size: %d", output_size);
    NN_INFO_PRINTF("Result: average is %f", output[0]);
    // NOTE: 11.95 instead of 12 because of errors due quantization
    assert(fabs(output[0] - 11.95) < EPSILON);

    free(input.dim);
    free(input.input_tensor);
    free(output);
}

int
main()
{
    NN_INFO_PRINTF("Usage:\niwasm --native-lib=./libwasi_nn_tflite.so "
                   "--wasi-nn-graph=encoding:target:model_path1:model_path2:..."
                   ":model_pathN test_tensorflow.wasm\"");

    NN_INFO_PRINTF("################### Testing quantized model...");
    test_average_quantized();

    NN_INFO_PRINTF("Tests: passed!");
    return 0;
}
