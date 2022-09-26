#ifndef WASI_NN_WASM_H
#define WASI_NN_WASM_H

#include "wasi_nn_common.h"

/**
 * Following definition from:
 * [Aug 10th, 2022]
 * https://github.com/WebAssembly/wasi-nn/blob/e5e1a6c31f424c7cd63026cd270e9746775675a0/wasi-nn.wit.md
 */

/* The graph initialization data. */

// This consists of an array of buffers because implementing backends may encode
// their graph IR in parts (e.g., OpenVINO stores its IR and weights
// separately).
typedef struct {
    uint8_t *buf;
    uint32_t size;
} graph_builder;

typedef struct {
    graph_builder *buf;
    uint32_t size;
} graph_builder_array;

/* The dimensions of a tensor. */

// The array length matches the tensor rank and each element in the array
// describes the size of each dimension.
typedef struct {
    uint32_t *buf;
    uint32_t size;
} tensor_dimensions;

/* The tensor data. */

// Initially conceived as a sparse representation, each empty cell would be
// filled with zeros and the array length must match the product of all of the
// dimensions and the number of bytes in the type (e.g., a 2x2 tensor with
// 4-byte f32 elements would have a data array of length 16). Naturally, this
// representation requires some knowledge of how to lay out data in
// memory--e.g., using row-major ordering--and could perhaps be improved.
typedef uint8_t *tensor_data;

/* A tensor. */

typedef struct {
    // Describe the size of the tensor (e.g., 2x2x2x2 -> [2, 2, 2, 2]). To
    // represent a tensor containing a single value, use `[1]` for the tensor
    // dimensions.
    tensor_dimensions *dimensions;
    // Describe the type of element in the tensor (e.g., f32).
    tensor_type type;
    // Contains the tensor data.
    tensor_data data;
} tensor;

/**
 * @brief Load an opaque sequence of bytes to use for inference.
 *
 * @param builder   Builder of the model.
 * @param encoding  Type of encoding of the model.
 * @param target    Execution target.
 * @param graph     Graph.
 * @return error    Execution status.
 */
error
load(graph_builder_array *builder, graph_encoding encoding,
     execution_target target, graph *graph) __attribute__((import_module("wasi_nn")));

/**
 * @brief Create an execution instance of a loaded graph.
 *
 * @param graph                     Initialialize execution context of graph
 * `graph`.
 * @param graph_execution_context   Graph execution context.
 * @return error
 */
error
init_execution_context(graph graph, graph_execution_context *ctx) __attribute__((import_module("wasi_nn")));

/**
 * @brief Define the inputs to use for inference.
 *
 * @param ctx       Execution context.
 * @param index     Input tensor index.
 * @param tensor    Input tensor.
 * @return error    Execution status.
 */
error
set_input(graph_execution_context ctx, uint32_t index, tensor *tensor) __attribute__((import_module("wasi_nn")));

/**
 * @brief Compute the inference on the given inputs.
 *
 * @param ctx       Execution context.
 * @return error    Execution status.
 */
error
compute(graph_execution_context ctx) __attribute__((import_module("wasi_nn")));

/**
 * @brief Extract the outputs after inference.
 *
 * @param ctx               Execution context.
 * @param index             Index of the output tensor.
 * @param tensor_data       Address
 * @param tensor_data_size  `tensor_data` maximum size.
 * @return error            Execution status.
 */
error
get_output(graph_execution_context ctx, uint32_t index, tensor_data data,
           uint32_t *data_size) __attribute__((import_module("wasi_nn")));

#endif
