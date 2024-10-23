# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

include(FetchContent)

set(TFLITE_SOURCE_DIR "${WAMR_ROOT_DIR}/core/deps/tensorflow-src")
if(EXISTS ${TFLITE_SOURCE_DIR})
  message("Use existed source code under ${TFLITE_SOURCE_DIR}")
  FetchContent_Declare(
    tensorflow_lite
    SOURCE_DIR     ${TFLITE_SOURCE_DIR}
    SOURCE_SUBDIR  tensorflow/lite
  )
else()
  message("download source code and store it at ${TFLITE_SOURCE_DIR}")
  FetchContent_Declare(
    tensorflow_lite
    GIT_REPOSITORY https://github.com/tensorflow/tensorflow.git
    GIT_TAG        v2.12.0
    GIT_SHALLOW    ON
    GIT_PROGRESS   ON
    SOURCE_DIR     ${TFLITE_SOURCE_DIR}
    SOURCE_SUBDIR  tensorflow/lite
    PATCH_COMMAND  git apply ${CMAKE_CURRENT_LIST_DIR}/add_telemetry.patch
  )
endif()


if(WAMR_BUILD_WASI_NN_ENABLE_GPU EQUAL 1)
  set(TFLITE_ENABLE_GPU ON)
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(TFLITE_ENABLE_XNNPACK OFF)
endif()

FetchContent_MakeAvailable(tensorflow_lite)
