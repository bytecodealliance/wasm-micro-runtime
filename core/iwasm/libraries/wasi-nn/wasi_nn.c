#include "wasi_nn.h"
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "wasm_export.h"

#include "lib_run_inference.hpp"

/**
 * @brief loader of tensorflow
 *
 * @param builder array of 2 pointers: first its the buffer, second its the size
 */
void
load_tensorflow(wasm_module_inst_t instance, graph_builder_array builder)
{
    printf("Loading tensorflow...\n");
    for (int i = 0; i < 2; ++i)
        builder[i] = (graph_builder)wasm_runtime_addr_app_to_native(instance,
                                                                    builder[i]);
}

uint32_t
wasi_nn_load(wasm_exec_env_t exec_env, uint32_t builder, uint32_t encoding)
{
    printf("Inside wasi_nn_load!\n\n");
    wasm_module_inst_t instance = wasm_runtime_get_module_inst(exec_env);
    graph_builder_array buf =
        (graph_builder_array)wasm_runtime_addr_app_to_native(instance, builder);
    switch ((graph_encoding)encoding) {
        case openvino:
            return invalid_argument;
        case tensorflow:
            load_tensorflow(instance, buf);
            break;
        case onnx:
            return invalid_argument;
    }
    return _load(buf, (graph_encoding)encoding);
}

void
wasi_nn_init_execution_context()
{}

uint32_t
wasi_nn_set_input(
    wasm_exec_env_t exec_env, graph_execution_context context, uint32_t index,
    uint32_t *input_tensor_size, uint32_t input_tensor_type,
    uint32_t *input_tensor) // Replaced struct by values inside of
                            // it as WASMR does not support structs
{
    printf("Inside wasi_nn_set_input!\n\n");

    wasm_module_inst_t instance = wasm_runtime_get_module_inst(exec_env);
    tensor_data data =
        (tensor_data)wasm_runtime_addr_app_to_native(instance, input_tensor);
    tensor_dimensions dimensions =
        (tensor_dimensions)wasm_runtime_addr_app_to_native(instance,
                                                           input_tensor_size);

    tensor_type type = (tensor_type)wasm_runtime_addr_app_to_native(
        instance, input_tensor_type);

    tensor tensor_struct = { .dimensions = dimensions,
                             .type = type,
                             .data = data };

    return _set_input(tensor_struct);
}

void
wasi_nn_compute()
{}

void
wasi_nn_get_output()
{}

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, wasi_nn_##func_name, signature, NULL }
/* clang-format on */

static NativeSymbol native_symbols_wasi_nn[] = {
    REG_NATIVE_FUNC(load, "(ii)i"),
    REG_NATIVE_FUNC(set_input, "(ii*i*)i"),
};

uint32_t
get_wasi_nn_export_apis(NativeSymbol **p_libc_wasi_apis)
{
    *p_libc_wasi_apis = native_symbols_wasi_nn;
    return sizeof(native_symbols_wasi_nn) / sizeof(NativeSymbol);
}
