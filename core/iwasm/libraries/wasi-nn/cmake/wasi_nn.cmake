# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

# Find tensorflow-lite
find_package(tensorflow_lite REQUIRED)

set(WASI_NN_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

#
# wasi-nn general
add_library(
  wasi-nn-general
  SHARED
    ${WASI_NN_ROOT}/src/wasi_nn.c
    ${WASI_NN_ROOT}/src/utils/wasi_nn_app_native.c
)
target_include_directories(
  wasi-nn-general
  PUBLIC
    ${WASI_NN_ROOT}/include
    ${WASI_NN_ROOT}/src
    ${WASI_NN_ROOT}/src/utils
)
target_link_libraries(
  wasi-nn-general
  PUBLIC
    libiwasm
)
target_compile_definitions(
  wasi-nn-general
  PUBLIC
   $<$<CONFIG:Debug>:NN_LOG_LEVEL=0>
   $<$<CONFIG:Release>:NN_LOG_LEVEL=2>
)

#
# wasi-nn backends
add_library(
  wasi-nn-tflite
  SHARED
    ${WASI_NN_ROOT}/src/wasi_nn_tensorflowlite.cpp
)
#target_link_options(
#  wasi-nn-tflite
#  PRIVATE
#    -Wl,--whole-archive libwasi-nn-general.a
#    -Wl,--no-whole-archive
#)
target_link_libraries(
  wasi-nn-tflite
  PUBLIC
    tensorflow-lite
    wasi-nn-general
)
