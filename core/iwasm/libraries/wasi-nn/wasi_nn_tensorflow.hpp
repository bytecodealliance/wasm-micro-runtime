#ifndef WASI_NN_TENSORFLOW_HPP
#define WASI_NN_TENSORFLOW_HPP

#include <stdio.h>

#include "wasi_nn.h"
#include "logger.h"

#ifdef __cplusplus
extern "C" {
#endif

error
_load(graph_builder_array builder, graph_encoding encoding,
      execution_target target, graph *graph);

error
_init_execution_context(graph graph);

error
_set_input(graph_execution_context ctx, uint32_t index, tensor *t);

error
_compute(graph_execution_context ctx);

error
_get_output(graph_execution_context context, uint32_t index, tensor_data data,
            uint32_t *data_size);

#ifdef __cplusplus
}
#endif

#endif
