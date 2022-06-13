#ifndef LIB_RUN_INFERENCE_HPP
#define LIB_RUN_INFERENCE_HPP

#include <stdio.h>

#include "wasi_nn.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t _load(graph_builder_array builder,  graph_encoding encoding);

#ifdef __cplusplus
}
#endif

#endif
