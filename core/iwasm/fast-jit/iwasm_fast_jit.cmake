# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (IWASM_FAST_JIT_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_FAST_JIT=1)

include_directories (${IWASM_FAST_JIT_DIR})

file (GLOB c_source_jit ${IWASM_FAST_JIT_DIR}/*.c ${IWASM_FAST_JIT_DIR}/fe/*.c)

if (WAMR_BUILD_TARGET STREQUAL "X86_64" OR WAMR_BUILD_TARGET STREQUAL "AMD_64")
  file (GLOB_RECURSE c_source_jit_cg ${IWASM_FAST_JIT_DIR}/cg/x86-64/*.c)
else ()
  message (FATAL_ERROR "Fast JIT codegen for target ${WAMR_BUILD_TARGET} isn't implemented")
endif ()

set (IWASM_FAST_JIT_SOURCE ${c_source_jit} ${c_source_jit_cg})
