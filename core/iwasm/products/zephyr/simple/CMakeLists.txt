# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

enable_language (ASM)

set (PLATFORM "zephyr")

# Build as X86_32 by default, change to "ARM[sub]", "THUMB[sub]", "MIPS" or "XTENSA"
# if we want to support arm, thumb, mips or xtensa
if (NOT DEFINED BUILD_TARGET)
  set (BUILD_TARGET "X86_32")
endif ()

if (NOT DEFINED WASM_ENABLE_INTERP)
  # Enable Interpreter by default
  set (WASM_ENABLE_INTERP 1)
endif ()

if (NOT DEFINED WASM_ENABLE_AOT)
  # Enable AOT by default.
  set (WASM_ENABLE_AOT 1)
endif ()

if (NOT DEFINED WASM_ENABLE_WASI)
  # Disable WASI support by default
  set (WASM_ENABLE_WASI 0)
endif ()

set (WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wamr)
# include the build config template makefile
include (${WAMR_ROOT_DIR}/cmake/config_common.cmake)

set (IWASM_ROOT ${WAMR_ROOT_DIR}/core/iwasm)
set (SHARED_LIB_ROOT ${WAMR_ROOT_DIR}/core/shared-lib)

include (${IWASM_ROOT}/runtime/utils/utils.cmake)
include (${IWASM_ROOT}/runtime/vmcore-wasm/vmcore.cmake)
include (${IWASM_ROOT}/lib/native/libc/wasmtime-wasi-c/wasi.cmake)
include (${IWASM_ROOT}/lib/native/base/wasm_lib_base.cmake)
include (${IWASM_ROOT}/lib/native/libc/wasm_libc.cmake)
include (${SHARED_LIB_ROOT}/platform/${PLATFORM}/shared_platform.cmake)
include (${SHARED_LIB_ROOT}/mem-alloc/mem_alloc.cmake)
include (${SHARED_LIB_ROOT}/utils/shared_utils.cmake)
if (WASM_ENABLE_AOT EQUAL 1)
include (${IWASM_ROOT}/runtime/aot/runtime/aot_runtime.cmake)
endif ()

include_directories (${IWASM_ROOT}/runtime/platform/include)

set (VM_LIB_SRCS
     ${WASM_PLATFORM_LIB_SOURCE}
     ${WASM_UTILS_LIB_SOURCE}
     ${VMCORE_LIB_SOURCE}
     ${AOT_RUNTIME_SOURCE}
     ${WASI_LIB_SOURCE}
     ${WASM_LIB_BASE_DIR}/base_lib_export.c
     ${WASM_LIBC_SOURCE}
     ${PLATFORM_SHARED_SOURCE}
     ${MEM_ALLOC_SHARED_SOURCE}
     ${UTILS_SHARED_SOURCE})

target_sources(app PRIVATE
               ${VM_LIB_SRCS}
               src/main.c
               src/ext_lib_export.c)

