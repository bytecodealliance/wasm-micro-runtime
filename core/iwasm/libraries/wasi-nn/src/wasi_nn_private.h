/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WASI_NN_PRIVATE_H
#define WASI_NN_PRIVATE_H

#include "wasi_nn_types.h"
#include "wasm_export.h"

typedef struct {
    bool is_model_loaded;
    graph_encoding backend;
    void *backend_ctx;
} WASINNContext;

#endif
