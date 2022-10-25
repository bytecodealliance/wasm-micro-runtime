# Copyright (C) 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (LIB_WASI_THREADS_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_LIB_WASI_THREADS=1)

include_directories(${LIB_WASI_THREADS_DIR})

file (GLOB source_all ${LIB_WASI_THREADS_DIR}/*.c)

set (LIB_WASI_THREADS_SOURCE ${source_all})

