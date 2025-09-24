# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

if (NOT DEFINED WAMR_BUILD_PLATFORM)
  set (WAMR_BUILD_PLATFORM "linux")
endif ()

enable_language (ASM)

# Usually, test cases should identify their unique
# complation flags to implement their test plan

# Set WAMR_BUILD_TARGET, currently values supported:
# "X86_64", "AMD_64", "X86_32", "ARM_32", "MIPS_32", "XTENSA_32"
if (NOT DEFINED WAMR_BUILD_TARGET)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Build as X86_64 by default in 64-bit platform
    set (WAMR_BUILD_TARGET "X86_64")
  else ()
    # Build as X86_32 by default in 32-bit platform
    set (WAMR_BUILD_TARGET "X86_32")
  endif ()
endif ()

set (WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

# include the build config template file
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

include_directories (${SHARED_DIR}/include
                     ${IWASM_DIR}/include)

include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)

# Add helper classes
include_directories(${CMAKE_CURRENT_LIST_DIR}/common)

# config_common.cmake only sets up the llvm environment when
# JIT is enabled. but in unit tests, we need llvm environment
# for aot compilation.
if (NOT DEFINED LLVM_DIR)
  set (LLVM_SRC_ROOT "${WAMR_ROOT_DIR}/core/deps/llvm")
  set (LLVM_BUILD_ROOT "${LLVM_SRC_ROOT}/build")
  if (NOT EXISTS "${LLVM_BUILD_ROOT}")
      message (FATAL_ERROR "Cannot find LLVM dir: ${LLVM_BUILD_ROOT}")
  endif ()
  set (CMAKE_PREFIX_PATH "${LLVM_BUILD_ROOT};${CMAKE_PREFIX_PATH}")
  set (LLVM_DIR ${LLVM_BUILD_ROOT}/lib/cmake/llvm)
endif ()

message(STATUS "unit_common.cmake included")
