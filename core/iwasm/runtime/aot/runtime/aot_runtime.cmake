# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (AOT_RUNTIME_DIR ${CMAKE_CURRENT_LIST_DIR})
set (COMPILER_RT_BUILTINS_DIR ${AOT_RUNTIME_DIR}/llvm-compiler-rt/lib/builtins)

include_directories(${AOT_RUNTIME_DIR})

file (GLOB source_aot_runtime ${AOT_RUNTIME_DIR}/*.c)

if (${BUILD_TARGET} STREQUAL "X86_64" OR ${BUILD_TARGET} STREQUAL "AMD_64")
  file (GLOB source_compiler_rt "")
elseif (${BUILD_TARGET} STREQUAL "X86_32")
  file (GLOB source_compiler_rt ${COMPILER_RT_BUILTINS_DIR}/i386/*.S)
elseif (${BUILD_TARGET} MATCHES "ARM.*")
  file (GLOB source_compiler_rt ${COMPILER_RT_BUILTINS_DIR}/*.c)
elseif (${BUILD_TARGET} MATCHES "THUMB.*")
  file (GLOB source_compiler_rt ${COMPILER_RT_BUILTINS_DIR}/*.c)
elseif (${BUILD_TARGET} STREQUAL "MIPS")
  set (source_compiler_rt "")
elseif (${BUILD_TARGET} STREQUAL "XTENSA")
  set (source_compiler_rt "")
elseif (${BUILD_TARGET} STREQUAL "GENERAL")
  set (source_compiler_rt "")
endif ()

set (AOT_RUNTIME_SOURCE ${source_aot_runtime} ${source_compiler_rt})

