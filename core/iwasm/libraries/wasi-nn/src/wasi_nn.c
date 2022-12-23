/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "wasi_nn.h"
#include "wasi_nn_app_native.h"
#include "logger.h"
#include "wasi_nn_tensorflowlite.hpp"

#include "bh_platform.h"
#include "wasm_export.h"

/* Definition of 'wasi_nn.h' structs in WASM app format (using offset) */

typedef error (*LOAD)(graph_builder_array *, graph_encoding, execution_target,
                      graph *);
typedef error (*INIT_EXECUTION_CONTEXT)(graph, graph_execution_context *);
typedef error (*SET_INPUT)(graph_execution_context, uint32_t, tensor_wasm *);
typedef error (*COMPUTE)(graph_execution_context);
typedef error (*GET_OUTPUT)(graph_execution_context, uint32_t, tensor_data,
                            uint32_t *);

typedef struct {
    LOAD load;
    INIT_EXECUTION_CONTEXT init_execution_context;
    SET_INPUT set_input;
    COMPUTE compute;
    GET_OUTPUT get_output;
} api_function;

/* Global variables */

static bool is_initialized = false;

static graph_encoding current_encoding;

static api_function lookup[] = {
    { NULL, NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL },
    { tensorflowlite_load, tensorflowlite_init_execution_context,
      tensorflowlite_set_input, tensorflowlite_compute,
      tensorflowlite_get_output }
};

/* Utils */

static bool
is_encoding_implemented(graph_encoding encoding)
{
    return lookup[encoding].load && lookup[encoding].init_execution_context
           && lookup[encoding].set_input && lookup[encoding].compute
           && lookup[encoding].get_output;
}

static error
is_model_initialized()
{
    if (!is_initialized) {
        NN_ERR_PRINTF("Model not initialized.");
        return invalid_argument;
    }
    return success;
}

/* WASI-NN implementation */

error
wasi_nn_load(wasm_exec_env_t exec_env, graph_builder_array_wasm *builder,
             graph_encoding encoding, execution_target target, graph *graph)
{
    NN_DBG_PRINTF("Running wasi_nn_load [encoding=%d, target=%d]...", encoding,
                  target);

    if (!is_encoding_implemented(encoding)) {
        NN_ERR_PRINTF("Encoding not supported.");
        return invalid_argument;
    }

    wasm_module_inst_t instance = wasm_runtime_get_module_inst(exec_env);
    bh_assert(instance);

    error res;
    graph_builder_array builder_native;
    if (success
        != (res = graph_builder_array_app_native(instance, builder,
                                                 &builder_native)))
        return res;

    if (!wasm_runtime_validate_native_addr(instance, graph, sizeof(graph))) {
        NN_ERR_PRINTF("graph is invalid");
        return invalid_argument;
    }

    current_encoding = encoding;
    is_initialized = true;

    res = lookup[current_encoding].load(&builder_native, current_encoding,
                                        target, graph);

    // XXX: Free intermediate structure pointers
    wasm_runtime_free(builder_native.buf);

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
    error res;
    if (success != (res = is_model_initialized()))
        return res;

    res = lookup[current_encoding].init_execution_context(graph, ctx);
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

    error res;
    if (success != (res = is_model_initialized()))
        return res;

    wasm_module_inst_t instance = wasm_runtime_get_module_inst(exec_env);
    bh_assert(instance);

    tensor input_tensor_native;
    if (success
        != (res = tensor_app_native(instance, input_tensor,
                                    &input_tensor_native)))
        return res;

    res = lookup[current_encoding].set_input(ctx, index, &input_tensor_native);

    // XXX: Free intermediate structure pointers
    wasm_runtime_free(input_tensor_native.dimensions);

    NN_DBG_PRINTF("wasi_nn_set_input finished with status %d", res);
    return res;
}

error
wasi_nn_compute(wasm_exec_env_t exec_env, graph_execution_context ctx)
{
    NN_DBG_PRINTF("Running wasi_nn_compute [ctx=%d]...", ctx);
    error res;
    if (success != (res = is_model_initialized()))
        return res;

    res = lookup[current_encoding].compute(ctx);
    NN_DBG_PRINTF("wasi_nn_compute finished with status %d", res);
    return res;
}

error
wasi_nn_get_output(wasm_exec_env_t exec_env, graph_execution_context ctx,
                   uint32_t index, tensor_data output_tensor,
                   uint32_t *output_tensor_size)
{
    NN_DBG_PRINTF("Running wasi_nn_get_output [ctx=%d, index=%d]...", ctx,
                  index);
    error res;
    if (success != (res = is_model_initialized()))
        return res;

    res = lookup[current_encoding].get_output(ctx, index, output_tensor,
                                              output_tensor_size);
    NN_DBG_PRINTF("wasi_nn_get_output finished with status %d [data_size=%d]",
                  res, *output_tensor_size);
    return res;
}

/* Register WASI-NN in WAMR */

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
