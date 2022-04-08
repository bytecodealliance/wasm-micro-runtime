# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (IWASM_GC_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_GC=1)

if (WAMR_TEST_GC EQUAL 1)
  add_definitions (-DGC_MANUALLY=1 -DGC_IN_EVERY_ALLOCATION=1)
endif ()

include_directories (${IWASM_GC_DIR})

file (GLOB_RECURSE source_all ${IWASM_GC_DIR}/*.c)

set (IWASM_GC_SOURCE ${source_all})

