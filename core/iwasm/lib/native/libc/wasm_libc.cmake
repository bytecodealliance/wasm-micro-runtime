# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (WASM_LIBC_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${WASM_LIBC_DIR})


file (GLOB source_all ${WASM_LIBC_DIR}/*.c)

set (WASM_LIBC_SOURCE ${source_all})

