# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 2.8)

project (iwasm)

set (PLATFORM "linux-sgx")

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

add_definitions(-DUSE_SGX=1)
add_definitions(-DWASM_ENABLE_LOG=0)
add_definitions(-Dbh_printf=bh_printf_sgx)
add_definitions(-Dvprintf=bh_vprintf_sgx)

# Set BUILD_TARGET
if (NOT DEFINED BUILD_TARGET)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Build as X86_64 by default in 64-bit platform
    set (BUILD_TARGET "X86_64")
  else ()
    # Build as X86_32 by default in 32-bit platform
    set (BUILD_TARGET "X86_32")
  endif ()
endif ()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

if (NOT DEFINED WASM_ENABLE_INTERP)
  # Enable Interpreter by default
  set (WASM_ENABLE_INTERP 1)
endif ()

if (NOT DEFINED WASM_ENABLE_AOT)
  # Disable AOT by default.
  set (WASM_ENABLE_AOT 0)
endif ()

if (NOT DEFINED WASM_ENABLE_JIT)
  # Disable JIT by default.
  set (WASM_ENABLE_JIT 0)
endif ()

if (NOT DEFINED WASM_ENABLE_WASI)
  # Disable wasi support by default
  set (WASM_ENABLE_WASI 0)
endif ()

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -ffunction-sections -fdata-sections \
                                     -Wall -Wno-unused-parameter -Wno-pedantic")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdinc -fvisibility=hidden -fpie -ffunction-sections -fdata-sections")

set (WAMR_ROOT_DIR ../../../..)
# include the build config template file
include (${WAMR_ROOT_DIR}/cmake/config_common.cmake)

set (SHARED_LIB_DIR ../../../shared-lib)

include_directories (.
                     ../../runtime/include
                     ../../runtime/platform/include
                     ${SHARED_LIB_DIR}/include
                     $ENV{SGX_SDK}/include
                     $ENV{SGX_SDK}/include/tlibc
                     $ENV{SGX_SDK}/include/libcxx)

enable_language (ASM)

include (../../runtime/utils/utils.cmake)
include (../../runtime/vmcore-wasm/vmcore.cmake)
include (../../lib/native/libc/wasmtime-wasi-c/wasi.cmake)
include (../../lib/native/base/wasm_lib_base.cmake)
include (../../lib/native/libc/wasm_libc.cmake)
include (${SHARED_LIB_DIR}/platform/${PLATFORM}/shared_platform.cmake)
include (${SHARED_LIB_DIR}/mem-alloc/mem_alloc.cmake)
include (${SHARED_LIB_DIR}/utils/shared_utils.cmake)

if (WASM_ENABLE_AOT EQUAL 1)
  include (../../runtime/aot/runtime/aot_runtime.cmake)
  if (WASM_ENABLE_JIT EQUAL 1)
    include (../../runtime/aot/compiler/aot_compiler.cmake)
  endif ()
endif ()

add_library (vmlib
             ${WASM_PLATFORM_LIB_SOURCE}
             ${WASM_UTILS_LIB_SOURCE}
             ${VMCORE_LIB_SOURCE}
             ${WASI_LIB_SOURCE}
             ${WASM_LIB_BASE_DIR}/base_lib_export.c
             ${WASM_LIBC_SOURCE}
             ${PLATFORM_SHARED_SOURCE}
             ${MEM_ALLOC_SHARED_SOURCE}
             ${UTILS_SHARED_SOURCE}
             ${AOT_RUNTIME_SOURCE}
             ${AOT_COMPILER_SOURCE})

add_library (extlib ext_lib_export.c)

set (copy_libs_cmd cp -a libvmlib.a libextlib.a ../enclave-sample/)
add_custom_target (copy_libs_to_enclave ALL COMMAND ${copy_libs_cmd} DEPENDS vmlib extlib)

