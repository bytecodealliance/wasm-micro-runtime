# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (IWASM_INTERP_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_INTERP=1)

include_directories(${IWASM_INTERP_DIR})

file (GLOB_RECURSE source_all ${IWASM_INTERP_DIR}/*.c)

set (IWASM_INTERP_SOURCE ${source_all})

