# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 3.14)

project(tflite_wasm)

include(ExternalProject)

# Only Download
ExternalProject_Add(tensorflow-download
    PREFIX tensorflow
    GIT_REPOSITORY https://github.com/tensorflow/tensorflow.git
    GIT_TAG        v2.13.0-rc2
    GIT_PROGRESS   ON
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/tensorflow
    UPDATE_COMMAND git restore .
    PATCH_COMMAND  git apply ${CMAKE_CURRENT_SOURCE_DIR}/tflite_bazel_building.patch
                     && ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/bazelrc.user ${CMAKE_CURRENT_SOURCE_DIR}/tensorflow/.bazelrc.user
                     && ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/absl_em_asm_int.patch ${CMAKE_CURRENT_SOURCE_DIR}/tensorflow/third_party/absl
    CONFIGURE_COMMAND ""
    BUILD_COMMAND  ""
    INSTALL_COMMAND ""
    TEST_COMMAND   ""
)

ExternalProject_Add(mobilenet-download
    URL  https://storage.googleapis.com/download.tensorflow.org/models/mobilenet_v1_2018_08_02/mobilenet_v1_1.0_224_quant.tgz
    URL_HASH SHA256=d32432d28673a936b2d6281ab0600c71cf7226dfe4cdcef3012555f691744166
    DOWNLOAD_NAME  mobilenet_v1_1.0_224_quant.tgz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    DOWNLOAD_NO_PROGRESS YES
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mobilenet
    CONFIGURE_COMMAND ""
    BUILD_COMMAND  ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/mobilenet/mobilenet_v1_1.0_224_quant.tflite ${CMAKE_CURRENT_BINARY_DIR}/mobilenet_v1_1.0_224_quant.tflite
)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/mobilenet_v1_1.0_224_quant.tflite)


set(WAMRC "${CMAKE_CURRENT_SOURCE_DIR}/../../../wamr-compiler/build/wamrc")
if(EXISTS ${WAMRC})
  message("-- Will generate .aot")
else()
  message("-- Will generate .wasm")
endif()

# benchmark_model
set(BENCHMARK "benchmark_model")

string(CONCAT WASM_OUTPUT ${BENCHMARK} ".wasm")
add_custom_command(
  OUTPUT ${WASM_OUTPUT}
  COMMAND bazel --output_user_root=build-user-output build -c opt --config=wasm //tensorflow/lite/tools/benchmark:benchmark_model-wasm
            && ${CMAKE_COMMAND} -E copy_if_different ./bazel-bin/tensorflow/lite/tools/benchmark/${WASM_OUTPUT} ${CMAKE_CURRENT_BINARY_DIR}/${WASM_OUTPUT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tensorflow
  DEPENDS tensorflow-download
  COMMENT "Generating ${WASM_OUTPUT} ..."
)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${WASM_OUTPUT})

if(EXISTS ${WAMRC})
  string(CONCAT AOT_OUTPUT ${BENCHMARK} ".aot")

  add_custom_command(
    OUTPUT ${AOT_OUTPUT}
    COMMAND ${WAMRC} -o ${AOT_OUTPUT} ${WASM_OUTPUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${WASM_OUTPUT}
    COMMENT "Generating ${AOT_OUTPUT} ..."
  )

  add_custom_target(${BENCHMARK} ALL DEPENDS ${AOT_OUTPUT})
else()
  add_custom_target(${BENCHMARK} ALL DEPENDS ${WASM_OUTPUT})
endif()
