# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

find_library(TENSORFLOW_LITE
  NAMES tensorflow-lite
  HINTS ${CMAKE_CURRENT_BINARY_DIR}/tensorflow-lite
  NO_DEFAULT_PATHS
)

if(NOT TENSORFLOW_LITE)
  if(NOT EXISTS "${WAMR_ROOT_DIR}/core/deps/tensorflow-src")
    execute_process(
      COMMAND "${WAMR_ROOT_DIR}/core/deps/install_tensorflow.sh"
      RESULT_VARIABLE TENSORFLOW_RESULT
    )
  else()
    message("Tensorflow is already downloaded.")
  endif()

  set(TENSORFLOW_SOURCE_DIR "${WAMR_ROOT_DIR}/core/deps/tensorflow-src")

  if(WAMR_BUILD_WASI_NN_ENABLE_GPU EQUAL 1)
    # Tensorflow specific:
    # * https://www.tensorflow.org/lite/guide/build_cmake#available_options_to_build_tensorflow_lite
    set (TFLITE_ENABLE_GPU ON)
  endif()

  if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    set (TFLITE_ENABLE_XNNPACK OFF)
  endif()

  add_subdirectory(
    "${TENSORFLOW_SOURCE_DIR}/tensorflow/lite"
    "${CMAKE_CURRENT_BINARY_DIR}/tensorflow-lite"
    EXCLUDE_FROM_ALL
  )
else ()
  message(STATUS "TensorFlow Lite library found: ${TENSORFLOW_LITE}")
  set(TENSORFLOW_SOURCE_DIR "${WAMR_ROOT_DIR}/core/deps/tensorflow-src")
endif()

set(TENSORFLOW_LITE_INCLUDE_DIR "${TENSORFLOW_SOURCE_DIR}/tensorflow/lite")
set(FLATBUFFER_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/flatbuffers/include")

include_directories(${TENSORFLOW_SOURCE_DIR})
include_directories(${FLATBUFFER_INCLUDE_DIR})
link_directories(${CMAKE_CURRENT_BINARY_DIR}/tensorflow-lite)
