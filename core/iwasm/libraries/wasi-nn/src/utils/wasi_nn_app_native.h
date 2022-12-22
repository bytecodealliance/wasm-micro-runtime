/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WASI_NN_APP_NATIVE
#define WASI_NN_APP_NATIVE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "wasi_nn.h"
#include "logger.h"

#include "bh_platform.h"
#include "wasm_export.h"

int
graph_builder_app_native(wasm_module_inst_t instance,
                         graph_builder_array_wasm *builder,
                         graph_builder_array *builder_native);

int
tensor_app_native(wasm_module_inst_t instance, tensor_wasm *input_tensor,
                  tensor *input_tensor_native);

#endif
