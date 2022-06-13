#ifndef WASI_NN_H
#define WASI_NN_H

#include <stdint.h>

/**
 * Following definition from:
 * https://github.com/WebAssembly/wasi-nn/blob/c557b2e9f84b6630f13b3185b43607f0388343b2/phases/ephemeral/witx/wasi_ephemeral_nn.witx
 */

typedef uint32_t buffer_size;

typedef uint32_t graph_execution_context;

typedef enum { success = 0, invalid_argument, missing_memory, busy } nn_erno;

typedef uint32_t *tensor_dimensions;

typedef enum { f16 = 0, f32, u8, i32 } tensor_type;

typedef uint8_t *tensor_data;

typedef struct {
    tensor_dimensions dimensions;
    tensor_type type;
    tensor_data data;
} tensor;

typedef uint8_t *graph_builder;

typedef graph_builder *graph_builder_array;

typedef enum { openvino = 0, tensorflow, onnx } graph_encoding;

typedef enum { cpu = 0, gpu, tpu } execution_target;

uint32_t
load(graph_builder_array builder, graph_encoding encoding);

void
init_execution_context();

uint32_t
set_input(graph_execution_context context, uint32_t index,
          uint32_t *input_tensor_size, uint32_t input_tensor_type,
          uint32_t *input_tensor);

void
compute();

void
get_output();

#endif
