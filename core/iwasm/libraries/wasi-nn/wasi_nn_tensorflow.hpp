#ifndef WASI_NN_TENSORFLOW_HPP
#define WASI_NN_TENSORFLOW_HPP

#include <stdio.h>

#include "wasi_nn.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t
_load(graph_builder_array builder, graph_encoding encoding);

uint32_t
_set_input(tensor input_tensor);

#ifdef __cplusplus
}
#endif

#endif
