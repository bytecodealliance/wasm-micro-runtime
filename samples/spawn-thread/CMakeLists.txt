# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 2.8)
project(spawn_thread)

################  runtime settings  ################
set(WAMR_BUILD_PLATFORM "linux")

# Resetdefault linker flags
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# WAMR features switch
set(WAMR_BUILD_TARGET "X86_64")
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_AOT 1)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_FAST_INTERP 1)
set(WAMR_BUILD_LIB_PTHREAD 1)

# compiling and linking flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -pie -fPIE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wformat -Wformat-security")

# build out vmlib
set(WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)

add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
################################################


################ wasm application ################
add_subdirectory(wasm-apps)

################ wamr runtime ################
include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)

set (RUNTIME_SOURCE_ALL
    ${CMAKE_CURRENT_LIST_DIR}/src/main.c
    ${UNCOMMON_SHARED_SOURCE}
)
add_executable (spawn_thread ${RUNTIME_SOURCE_ALL})
target_link_libraries(spawn_thread vmlib -lpthread -lm)