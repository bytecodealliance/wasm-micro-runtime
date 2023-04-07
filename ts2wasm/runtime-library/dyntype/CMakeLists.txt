#
# Copyright (C) 2023 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

cmake_minimum_required(VERSION 2.8)

project(libdyntype)

set(QUICKJS_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/../deps/quickjs)

include_directories(.)
include_directories(${QUICKJS_SRC_DIR})

set(LIB_QUICKJS
    ${QUICKJS_SRC_DIR}/cutils.c
    ${QUICKJS_SRC_DIR}/libregexp.c
    ${QUICKJS_SRC_DIR}/libunicode.c
    ${QUICKJS_SRC_DIR}/quickjs.c)

set(LIB_DYNTYPE
    dyntype.c)

add_library(quickjs ${LIB_QUICKJS})
add_library(dyntype ${LIB_DYNTYPE})
target_link_libraries(dyntype quickjs)

set (WAMR_BUILD_PLATFORM "linux")
set (WAMR_BUILD_TARGET X86_64)
set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_AOT 1)
include(${CMAKE_CURRENT_LIST_DIR}/../deps/wamr-gc/build-scripts/runtime_lib.cmake)

add_library(dyntype_export SHARED lib_dyntype_wrapper.c)
target_link_libraries(dyntype_export dyntype)
