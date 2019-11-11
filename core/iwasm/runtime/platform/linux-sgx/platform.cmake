# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

add_definitions (-D__POSIX__ -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=199309L)

set (PLATFORM_LIB_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(${PLATFORM_LIB_DIR})
include_directories(${PLATFORM_LIB_DIR}/../include)

file (GLOB_RECURSE source_all ${PLATFORM_LIB_DIR}/*.c)

set (WASM_PLATFORM_LIB_SOURCE ${source_all})

