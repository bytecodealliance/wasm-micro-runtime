/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasi_nn_app_native.h"

static int
graph_builder_app_native(wasm_module_inst_t instance,
                         graph_builder_array_wasm *builder,
                         graph_builder_array, )
{
    if (!wasm_runtime_validate_native_addr(instance, builder,
                                           sizeof(graph_builder_array_wasm)))
        return invalid_argument;

    if (!wasm_runtime_validate_app_addr(instance, builder->buf_offset,
                                        builder->size * sizeof(uint32_t)))
        return invalid_argument;

    NN_DBG_PRINTF("Graph builder array contains %d elements", builder->size);

    graph_builder_wasm *gb_wasm =
        (graph_builder_wasm *)wasm_runtime_addr_app_to_native(
            instance, builder->buf_offset);

    graph_builder *gb_native = (graph_builder *)wasm_runtime_malloc(
        builder->size * sizeof(graph_builder));
    if (gb_native == NULL)
        return missing_memory;

    for (int i = 0; i < builder->size; ++i) {
        if (!wasm_runtime_validate_app_addr(instance, gb_wasm[i].buf_offset,
                                            gb_wasm[i].size
                                                * sizeof(uint8_t))) {
            wasm_runtime_free(gb_native);
            return invalid_argument;
        }

        gb_native[i].buf = (uint8_t *)wasm_runtime_addr_app_to_native(
            instance, gb_wasm[i].buf_offset);
        gb_native[i].size = gb_wasm[i].size;

        NN_DBG_PRINTF("Graph builder %d contains %d elements", i,
                      gb_wasm[i].size);
    }

    graph_builder_array gba_native = { .buf = gb_native,
                                       .size = builder->size };
}

static int
tensor_app_native(wasm_module_inst_t instance, tensor_wasm *input_tensor,
                  tensor *input_tensor_native)
{
    if (!wasm_runtime_validate_native_addr(instance, input_tensor,
                                           sizeof(tensor_wasm)))
        return invalid_argument;

    if (!wasm_runtime_validate_app_addr(
            instance, input_tensor->dimensions_offset, sizeof(uint32_t)))
        return invalid_argument;

    tensor_dimensions_wasm *dimensions_w =
        (tensor_dimensions_wasm *)wasm_runtime_addr_app_to_native(
            instance, input_tensor->dimensions_offset);

    if (!wasm_runtime_validate_app_addr(instance, dimensions_w->buf_offset,
                                        dimensions_w->size * sizeof(uint32_t)))
        return invalid_argument;

    tensor_dimensions dimensions = {
        .buf = (uint32_t *)wasm_runtime_addr_app_to_native(
            instance, dimensions_w->buf_offset),
        .size = dimensions_w->size
    };

    NN_DBG_PRINTF("Number of dimensions: %d", dimensions.size);
    int total_elements = 1;
    for (int i = 0; i < dimensions.size; ++i) {
        NN_DBG_PRINTF("Dimension %d: %d", i, dimensions.buf[i]);
        total_elements *= dimensions.buf[i];
    }
    NN_DBG_PRINTF("Tensor type: %d", input_tensor->type);

    if (!wasm_runtime_validate_app_addr(instance, input_tensor->data_offset,
                                        total_elements))
        return invalid_argument;

    tensor tensor = { .type = input_tensor->type,
                      .dimensions = &dimensions,
                      .data = (uint8_t *)wasm_runtime_addr_app_to_native(
                          instance, input_tensor->data_offset) };
}