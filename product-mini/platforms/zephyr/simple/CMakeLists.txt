# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

enable_language (ASM)

set (WAMR_BUILD_PLATFORM "zephyr")

# Build as X86_32 by default, change to "ARM[sub]", "THUMB[sub]", "MIPS" or "XTENSA"
# if we want to support arm, thumb, mips or xtensa
if (NOT DEFINED WAMR_BUILD_TARGET)
  set (WAMR_BUILD_TARGET "X86_32")
endif ()

if (NOT DEFINED WAMR_BUILD_INTERP)
  # Enable Interpreter by default
  set (WAMR_BUILD_INTERP 1)
endif ()

if (NOT DEFINED WAMR_BUILD_AOT)
  # Enable AOT by default.
  set (WAMR_BUILD_AOT 1)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_BUILTIN)
  # Enable libc builtin support by default
  set (WAMR_BUILD_LIBC_BUILTIN 1)
endif ()

if (NOT DEFINED WAMR_BUILD_LIBC_WASI)
  # Disable libc wasi support by default
  set (WAMR_BUILD_LIBC_WASI 0)
endif ()

set (WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wamr)
set (SHARED_DIR ${WAMR_ROOT_DIR}/core/shared)
set (IWASM_DIR ${WAMR_ROOT_DIR}/core/iwasm)
set (APP_FRAMEWORK_DIR ${WAMR_ROOT_DIR}/core/app-framework)

# include the build config template makefile
include (${WAMR_ROOT_DIR}/build-scripts/config_common.cmake)

include_directories (${SHARED_DIR}/include
                     ${IWASM_DIR}/include)

include (${SHARED_DIR}/platform/${WAMR_BUILD_PLATFORM}/shared_platform.cmake)
include (${SHARED_DIR}/mem-alloc/mem_alloc.cmake)
include (${SHARED_DIR}/utils/shared_utils.cmake)
if (WAMR_BUILD_LIBC_BUILTIN EQUAL 1)
  include (${IWASM_DIR}/libraries/libc-builtin/libc_builtin.cmake)
endif ()
if (WAMR_BUILD_LIBC_WASI EQUAL 1)
  include (${IWASM_DIR}/libraries/libc-wasi/libc_wasi.cmake)
endif ()

include (${IWASM_DIR}/common/iwasm_common.cmake)

if (WAMR_BUILD_INTERP EQUAL 1 OR WAMR_BUILD_JIT EQUAL 1)
  include (${IWASM_DIR}/interpreter/iwasm_interp.cmake)
endif()

if (WAMR_BUILD_AOT EQUAL 1)
  include (${IWASM_DIR}/aot/iwasm_aot.cmake)
endif ()

set (VM_LIB_SRCS
     ${PLATFORM_SHARED_SOURCE}
     ${MEM_ALLOC_SHARED_SOURCE}
     ${UTILS_SHARED_SOURCE}
     ${LIBC_BUILTIN_SOURCE}
     ${LIBC_WASI_SOURCE}
     ${IWASM_COMMON_SOURCE}
     ${IWASM_INTERP_SOURCE}
     ${IWASM_AOT_SOURCE}
     ${IWASM_COMPL_SOURCE})

target_sources(app PRIVATE
               ${VM_LIB_SRCS}
               src/main.c)

