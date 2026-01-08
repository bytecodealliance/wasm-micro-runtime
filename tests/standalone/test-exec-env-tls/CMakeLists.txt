# Copyright (C) 2024 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(test_exec_env_tls)

################ runtime settings  ##############
string (TOLOWER ${CMAKE_HOST_SYSTEM_NAME} WAMR_BUILD_PLATFORM)
if (APPLE)
  add_definitions(-DBH_PLATFORM_DARWIN)
endif ()

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Set WAMR_BUILD_TARGET
if (NOT DEFINED WAMR_BUILD_TARGET)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)")
    set (WAMR_BUILD_TARGET "AARCH64")
  elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv64")
    set (WAMR_BUILD_TARGET "RISCV64")
  elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set (WAMR_BUILD_TARGET "X86_64")
  elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    set (WAMR_BUILD_TARGET "X86_32")
  else ()
    message(SEND_ERROR "Unsupported build target platform!")
  endif ()
endif ()

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Debug)
endif ()

# WAMR features - enable HW bound check for this test
set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_AOT 1)
set (WAMR_BUILD_LIBC_BUILTIN 1)

# compiling and linking flags
if (NOT (CMAKE_C_COMPILER MATCHES ".*clang.*" OR CMAKE_C_COMPILER_ID MATCHES ".*Clang"))
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
endif ()

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wformat -Wformat-security")

# build out libiwasm
set (WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

add_library(libiwasm STATIC ${WAMR_RUNTIME_LIB_SOURCE})
set_target_properties (libiwasm PROPERTIES OUTPUT_NAME iwasm)

################ test executable ################
add_executable (test_exec_env_tls test_exec_env_tls.c)

target_include_directories(test_exec_env_tls PRIVATE
    ${WAMR_ROOT_DIR}/core/iwasm/include
    ${WAMR_ROOT_DIR}/core/iwasm/common
    ${SHARED_DIR}/include
    ${PLATFORM_SHARED_DIR}
)

target_link_libraries(test_exec_env_tls libiwasm -lpthread -lm)

if (NOT APPLE)
  target_link_libraries(test_exec_env_tls -ldl)
endif ()
