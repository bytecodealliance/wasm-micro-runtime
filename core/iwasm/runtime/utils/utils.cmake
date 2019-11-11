# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (UTILS_LIB_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${UTILS_LIB_DIR})

file (GLOB_RECURSE source_all ${UTILS_LIB_DIR}/*.c )

set (WASM_UTILS_LIB_SOURCE ${source_all})

