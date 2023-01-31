# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (WASI_NN_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_WASI_NN=1)

include_directories (${WASI_NN_DIR})
include_directories (${WASI_NN_DIR}/src)
include_directories (${WASI_NN_DIR}/src/utils)

set (
    LIBC_WASI_NN_SOURCE
    ${WASI_NN_DIR}/src/wasi_nn.c
    ${WASI_NN_DIR}/src/wasi_nn_tensorflowlite.cpp
    ${WASI_NN_DIR}/src/utils/wasi_nn_app_native.c
)

set (TENSORFLOW_LIB tensorflow-lite)
