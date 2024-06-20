# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

if(WAMR_BUILD_WASI_NN_TFLITE EQUAL 1)
  # Find tensorflow-lite
  find_package(tensorflow_lite REQUIRED)
endif()

if(WAMR_BUILD_WASI_NN_OPENVINO EQUAL 1)
  if(NOT DEFINED ENV{OpenVINO_DIR})
    message(FATAL_ERROR
        "OpenVINO_DIR is not defined. "
        "Please follow https://docs.openvino.ai/2024/get-started/install-openvino.html,"
        "install openvino, and set environment variable OpenVINO_DIR."
        "Like OpenVINO_DIR=/usr/lib/openvino-2023.2/ cmake ..."
        "Or OpenVINO_DIR=/opt/intel/openvino/ cmake ..."
    )
  endif()

  list(APPEND CMAKE_MODULE_PATH $ENV{OpenVINO_DIR})
  # Find OpenVINO
  find_package(OpenVINO REQUIRED COMPONENTS Runtime)
endif()

#
# wasi-nn general
set(WASI_NN_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
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

# - tflite
if(WAMR_BUILD_WASI_NN_TFLITE EQUAL 1)
  add_library(
    wasi-nn-tflite
    SHARED
      ${WASI_NN_ROOT}/src/wasi_nn_tensorflowlite.cpp
  )
  target_link_libraries(
    wasi-nn-tflite
    PUBLIC
      tensorflow-lite
      wasi-nn-general
  )
endif()

# - openvino
if(WAMR_BUILD_WASI_NN_OPENVINO EQUAL 1)
  add_library(
    wasi-nn-openvino
    SHARED
      ${WASI_NN_ROOT}/src/wasi_nn_openvino.c
  )
  target_link_libraries(
    wasi-nn-openvino
    PUBLIC
      openvino::runtime
      openvino::runtime::c
      wasi-nn-general
  )
endif()