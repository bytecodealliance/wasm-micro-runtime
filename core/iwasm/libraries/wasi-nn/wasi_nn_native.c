#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "wasm_export.h"

#include "wasi_nn.h"
#include "wasi_nn_tensorflow.hpp"
#include "logger.h"

static uint8_t is_initialized;
static graph_encoding _encoding;

typedef struct {
    uint32_t buf_offset;
    uint32_t size;
} graph_builder_wasm;

typedef struct {
    uint32_t buf_offset;
    uint32_t size;
} graph_builder_array_wasm;

typedef struct {
    uint32_t dimensions_offset;
    tensor_type type;
    uint32_t data_offset;
} tensor_wasm;

typedef struct {
    uint32_t buf_offset;
    uint32_t size;
} tensor_dimensions_wasm;

/* WASI-NN implementation */

error
wasi_nn_load(wasm_exec_env_t exec_env, graph_builder_array_wasm *builder,
             graph_encoding encoding, execution_target target, graph *graph)
{
    NN_DBG_PRINTF("Running wasi_nn_load [encoding=%d, target]...", encoding,
                  target);
    wasm_module_inst_t instance = wasm_runtime_get_module_inst(exec_env);

    if (!wasm_runtime_validate_native_addr(instance, builder,
                                           sizeof(graph_builder_array_wasm)))
        return invalid_argument;

    if (!wasm_runtime_validate_app_addr(instance, builder->buf_offset,
                                        builder->size * sizeof(graph_builder)))
        return invalid_argument;

    NN_DBG_PRINTF("Graph builder array contains %d elements", builder->size);

    graph_builder *gb_n =
        (graph_builder *)malloc(builder->size * sizeof(graph_builder));

    graph_builder_wasm *gb_w =
        (graph_builder_wasm *)wasm_runtime_addr_app_to_native(
            instance, builder->buf_offset);
    for (int i = 0; i < builder->size; ++i) {
        if (!wasm_runtime_validate_app_addr(instance, gb_w[i].buf_offset,
                                            gb_w[i].size * sizeof(uint8_t)))
            return invalid_argument;

        gb_n[i].buf = (uint8_t *)wasm_runtime_addr_app_to_native(
            instance, gb_w[i].buf_offset);
        gb_n[i].size = gb_w[i].size;

        NN_DBG_PRINTF("Graph builder %d contains %d elements", i, gb_w[i].size);
    }

    graph_builder_array gba_n = { .buf = gb_n, .size = builder->size };

    if (!wasm_runtime_validate_native_addr(instance, graph, sizeof(graph)))
        return invalid_argument;

    switch (encoding) {
        case tensorflow:
            break;
        default:
            return invalid_argument;
    }

    _encoding = encoding;
    is_initialized = 1;

    error res = _load(gba_n, _encoding, target, graph);
    free(gb_n);
    NN_DBG_PRINTF("wasi_nn_load finished with status %d [graph=%d]", res,
                  *graph);
    return res;
}

error
wasi_nn_init_execution_context(wasm_exec_env_t exec_env, graph graph,
                               graph_execution_context *ctx)
{
    NN_DBG_PRINTF("Running wasi_nn_init_execution_context [graph=%d]...",
                  graph);
    error res = _init_execution_context(graph);
    *ctx = graph;
    NN_DBG_PRINTF(
        "wasi_nn_init_execution_context finished with status %d [ctx=%d]", res,
        *ctx);
    return res;
}

error
wasi_nn_set_input(wasm_exec_env_t exec_env, graph_execution_context ctx,
                  uint32_t index, tensor_wasm *input_tensor)
{
    NN_DBG_PRINTF("Running wasi_nn_set_input [ctx=%d, index=%d]...", ctx,
                  index);
    wasm_module_inst_t instance = wasm_runtime_get_module_inst(exec_env);

    if (!wasm_runtime_validate_native_addr(instance, input_tensor, sizeof(tensor_wasm)))
        return invalid_argument;

    if (!wasm_runtime_validate_app_addr(instance, input_tensor->dimensions_offset,
                                        sizeof(uint32_t)))
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

    error res = _set_input(ctx, index, &tensor);
    NN_DBG_PRINTF("wasi_nn_set_input finished with status %d", res);
    return res;
}

error
wasi_nn_compute(wasm_exec_env_t exec_env, graph_execution_context ctx)
{
    NN_DBG_PRINTF("Running wasi_nn_compute [ctx=%d]...", ctx);
    error res = _compute(ctx);
    NN_DBG_PRINTF("wasi_nn_compute finished with status %d", res);
    return res;
}

error
wasi_nn_get_output(wasm_exec_env_t exec_env, graph_execution_context ctx,
                   uint32_t index, tensor_data output_tensor, uint32_t *output_tensor_size)
{
    NN_DBG_PRINTF("Running wasi_nn_get_output [ctx=%d, index=%d]...", ctx,
                  index);
    error res = _get_output(ctx, index, output_tensor, output_tensor_size);
    NN_DBG_PRINTF("wasi_nn_get_output finished with status %d [data_size=%d]",
                  res, *output_tensor_size);
    return res;
}

/* Register native symbols and utility */

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, wasi_nn_##func_name, signature, NULL }
/* clang-format on */

static NativeSymbol native_symbols_wasi_nn[] = {
    REG_NATIVE_FUNC(load, "(*ii*)i"),
    REG_NATIVE_FUNC(init_execution_context, "(i*)i"),
    REG_NATIVE_FUNC(set_input, "(ii*)i"),
    REG_NATIVE_FUNC(compute, "(i)i"),
    REG_NATIVE_FUNC(get_output, "(ii**)i"),
};

uint32_t
get_wasi_nn_export_apis(NativeSymbol **p_libc_wasi_apis)
{
    *p_libc_wasi_apis = native_symbols_wasi_nn;
    return sizeof(native_symbols_wasi_nn) / sizeof(NativeSymbol);
}
