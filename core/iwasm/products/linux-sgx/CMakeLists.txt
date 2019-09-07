# Copyright (C) 2019 Intel Corporation.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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

# Enable repl mode if want to test spec cases
# add_definitions(-DWASM_ENABLE_REPL)

if (NOT ("$ENV{VALGRIND}" STREQUAL "YES"))
  add_definitions(-DNVALGRIND)
endif ()

# Currently build as 64-bit by default.
set (BUILD_AS_64BIT_SUPPORT "YES")

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
if (${BUILD_AS_64BIT_SUPPORT} STREQUAL "YES")
  # Add -fPIC flag if build as 64-bit
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -fPIC")
else ()
  add_definitions (-m32)
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
endif ()
endif ()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif (NOT CMAKE_BUILD_TYPE)
message ("CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})

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
include (../../lib/native/base/wasm_lib_base.cmake)
include (../../lib/native/libc/wasm_libc.cmake)
include (${SHARED_LIB_DIR}/platform/${PLATFORM}/shared_platform.cmake)
include (${SHARED_LIB_DIR}/mem-alloc/mem_alloc.cmake)
include (${SHARED_LIB_DIR}/utils/shared_utils.cmake)

add_library (vmlib
             ${WASM_PLATFORM_LIB_SOURCE}
             ${WASM_UTILS_LIB_SOURCE}
             ${VMCORE_LIB_SOURCE}
             ${WASM_LIB_BASE_DIR}/base_lib_export.c
             ${WASM_LIBC_SOURCE}
             ${PLATFORM_SHARED_SOURCE}
             ${MEM_ALLOC_SHARED_SOURCE}
             ${UTILS_SHARED_SOURCE})

add_library (extlib ext_lib_export.c)
