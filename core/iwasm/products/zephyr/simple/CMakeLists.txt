# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

enable_language (ASM)

add_definitions (-DNVALGRIND)

# Build as X86_32 by default, change to "ARM_32", "MIPS_32" or "XTENSA_32"
# if we want to support arm, mips or xtensa
if (NOT BUILD_TARGET)
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

include_directories (${IWASM_ROOT}/runtime/include
                     ${IWASM_ROOT}/runtime/platform/include
                     ${IWASM_ROOT}/runtime/platform/zephyr
                     ${IWASM_ROOT}/runtime/vmcore-wasm
                     ${SHARED_LIB_ROOT}/include
                     ${SHARED_LIB_ROOT}/platform/include
                     ${SHARED_LIB_ROOT}/platform/zephyr)

set (IWASM_SRCS ${IWASM_ROOT}/runtime/utils/wasm_hashmap.c
                ${IWASM_ROOT}/runtime/utils/wasm_log.c
                ${IWASM_ROOT}/runtime/utils/wasm_dlfcn.c
                ${IWASM_ROOT}/runtime/platform/zephyr/wasm_native.c
                ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_application.c
                ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_interp.c
                ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_loader.c
                ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_runtime.c
                ${IWASM_ROOT}/runtime/vmcore-wasm/invokeNative_general.c
                ${IWASM_ROOT}/lib/native/libc/libc_wrapper.c
                ${IWASM_ROOT}/lib/native/base/base_lib_export.c
                ${SHARED_LIB_ROOT}/platform/zephyr/bh_platform.c
                ${SHARED_LIB_ROOT}/platform/zephyr/bh_assert.c
                ${SHARED_LIB_ROOT}/platform/zephyr/bh_thread.c
                ${SHARED_LIB_ROOT}/platform/zephyr/bh_math.c
                ${SHARED_LIB_ROOT}/mem-alloc/bh_memory.c
                ${SHARED_LIB_ROOT}/mem-alloc/mem_alloc.c
                ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_kfc.c
                ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_alloc.c
                ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_hmu.c)

target_sources(app PRIVATE ${IWASM_SRCS} src/main.c src/ext_lib_export.c)
