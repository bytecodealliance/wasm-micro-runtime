# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 2.8)

project (iwasm)

set (PLATFORM "linux-sgx")

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

add_definitions(-DUSE_SGX=1)
add_definitions(-DOPS_INPUT_OUTPUT=1)
add_definitions(-DOPS_UNSAFE_BUFFERS=0)
add_definitions(-DWASM_ENABLE_LOG=0)
add_definitions(-Dbh_printf=bh_printf_sgx)

if (NOT ("$ENV{VALGRIND}" STREQUAL "YES"))
  add_definitions(-DNVALGRIND)
endif ()

# Set BUILD_TARGET, currently values supported:
if (NOT DEFINED BUILD_TARGET)
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Build as X86_64 by default in 64-bit platform
    set (BUILD_TARGET "X86_64")
  else ()
    # Build as X86_32 by default in 32-bit platform
    set (BUILD_TARGET "X86_32")
  endif ()
endif ()

string(TOUPPER ${BUILD_TARGET} BUILD_TARGET)

# Add definitions for the build target
if (BUILD_TARGET STREQUAL "X86_64")
  add_definitions(-DBUILD_TARGET_X86_64)
elseif (BUILD_TARGET STREQUAL "X86_32")
  add_definitions(-DBUILD_TARGET_X86_32)
else ()
  message (FATAL_ERROR "-- Build target isn't set")
endif ()

message ("-- Build as target ${BUILD_TARGET}")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif (NOT CMAKE_BUILD_TYPE)
message ("-- CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  if (BUILD_TARGET STREQUAL "X86_64")
    # Add -fPIC flag if build as 64-bit
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -fPIC")
  else ()
    add_definitions (-m32)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
  endif ()
endif ()

if (NOT DEFINED WASM_ENABLE_WASI)
  # Disable wasi support by default
  set (WASM_ENABLE_WASI 0)
endif ()

if (WASM_ENABLE_WASI EQUAL 1)
  add_definitions(-DWASM_ENABLE_WASI=1)
  add_definitions(-D_GNU_SOURCE)
  message ("-- WASI enabled")
else ()
  message ("-- WASI disabled")
endif ()

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -Wall -Wno-unused-parameter -Wno-pedantic")

set (SHARED_LIB_DIR ../../../shared-lib)

include_directories (.
                     ../../runtime/include
                     ../../runtime/platform/include
                     ${SHARED_LIB_DIR}/include
                     $ENV{SGX_SDK}/include)

enable_language (ASM)

include (../../runtime/platform/${PLATFORM}/platform.cmake)
include (../../runtime/utils/utils.cmake)
include (../../runtime/vmcore-wasm/vmcore.cmake)
include (../../runtime/wasmtime-wasi-c/wasi.cmake)
include (../../lib/native/base/wasm_lib_base.cmake)
include (../../lib/native/libc/wasm_libc.cmake)
include (${SHARED_LIB_DIR}/platform/${PLATFORM}/shared_platform.cmake)
include (${SHARED_LIB_DIR}/mem-alloc/mem_alloc.cmake)
include (${SHARED_LIB_DIR}/utils/shared_utils.cmake)

add_library (vmlib
             ${WASM_PLATFORM_LIB_SOURCE}
             ${WASM_UTILS_LIB_SOURCE}
             ${VMCORE_LIB_SOURCE}
             ${WASI_LIB_SOURCE}
             ${WASM_LIB_BASE_DIR}/base_lib_export.c
             ${WASM_LIBC_SOURCE}
             ${PLATFORM_SHARED_SOURCE}
             ${MEM_ALLOC_SHARED_SOURCE}
             ${UTILS_SHARED_SOURCE})

add_library (extlib ext_lib_export.c)

set (copy_libs_cmd cp -a libvmlib.a libextlib.a ../enclave-sample/)
add_custom_target (copy_libs_to_enclave ALL COMMAND ${copy_libs_cmd} DEPENDS vmlib extlib)

