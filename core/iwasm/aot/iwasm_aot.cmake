# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (IWASM_AOT_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_AOT=1)

include_directories (${IWASM_AOT_DIR})

file (GLOB c_source_all ${IWASM_AOT_DIR}/*.c)

set (IWASM_AOT_SOURCE ${c_source_all})

