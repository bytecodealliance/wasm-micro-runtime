# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

enable_language (ASM)

set (PLATFORM "zephyr")

add_definitions (-DNVALGRIND)

# Build as X86_32 by default, change to "ARM_32", "MIPS_32" or "XTENSA_32"
# if we want to support arm, mips or xtensa
if (NOT DEFINED BUILD_TARGET)
  set (BUILD_TARGET "X86_32")
endif ()

string(TOUPPER ${BUILD_TARGET} BUILD_TARGET)

if (BUILD_TARGET STREQUAL "X86_32")
  add_definitions(-DBUILD_TARGET_X86_32)
elseif (BUILD_TARGET STREQUAL "ARM_32")
  add_definitions(-DBUILD_TARGET_ARM_32)
elseif (BUILD_TARGET STREQUAL "MIPS_32")
  add_definitions(-DBUILD_TARGET_MIPS_32)
elseif (BUILD_TARGET STREQUAL "XTENSA_32")
  add_definitions(-DBUILD_TARGET_XTENSA_32)
else ()
  message (FATAL_ERROR "-- Build target isn't set")
endif ()

message ("-- Build as target ${BUILD_TARGET}")

set (IWASM_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/iwasm)
set (SHARED_LIB_ROOT ${IWASM_ROOT}/../shared-lib)

include (${IWASM_ROOT}/runtime/utils/utils.cmake)
include (${IWASM_ROOT}/runtime/vmcore-wasm/vmcore.cmake)
include (${IWASM_ROOT}/lib/native/libc/wasmtime-wasi-c/wasi.cmake)
include (${IWASM_ROOT}/lib/native/base/wasm_lib_base.cmake)
include (${IWASM_ROOT}/lib/native/libc/wasm_libc.cmake)
include (${SHARED_LIB_ROOT}/platform/${PLATFORM}/shared_platform.cmake)
include (${SHARED_LIB_ROOT}/mem-alloc/mem_alloc.cmake)
include (${SHARED_LIB_ROOT}/utils/shared_utils.cmake)

include_directories (${IWASM_ROOT}/runtime/platform/include)

set (VM_LIB_SRCS
     ${WASM_PLATFORM_LIB_SOURCE}
     ${WASM_UTILS_LIB_SOURCE}
     ${VMCORE_LIB_SOURCE}
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

