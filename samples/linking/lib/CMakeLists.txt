# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# compilation options
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_AOT 1)
set(WAMR_BUILD_LIBC_BUILTIN 0)
set(WAMR_BUILD_LIBC_WASI 1)

string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} WAMR_BUILD_PLATFORM)
set(WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
include(${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})

# # workaround for missing headers
include(${WAMR_ROOT_DIR}/core/shared/utils/uncommon/shared_uncommon.cmake)

get_property(WAMR_INCLUDE_PATHS_OUT DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
include(CMakePrintHelpers)
set(WAMR_INCLUDE_PATHS ${WAMR_INCLUDE_PATHS_OUT} PARENT_SCOPE)
cmake_print_variables(WAMR_INCLUDE_PATHS_OUT)
