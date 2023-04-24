# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

find_library(TENSORFLOW_LITE 
    NAMES tensorflow-lite
    HINTS ${CMAKE_LIBRARY_PATH})

if(NOT EXISTS ${TENSORFLOW_LITE})
    if (NOT EXISTS "${WAMR_ROOT_DIR}/core/deps/tensorflow-src")
        execute_process(COMMAND ${WAMR_ROOT_DIR}/core/deps/install_tensorflow.sh
                        RESULT_VARIABLE TENSORFLOW_RESULT
        )
    else ()
        message("Tensorflow is already downloaded.")
    endif()
    set(TENSORFLOW_SOURCE_DIR "${WAMR_ROOT_DIR}/core/deps/tensorflow-src")
    include_directories (${CMAKE_CURRENT_BINARY_DIR}/flatbuffers/include)
    include_directories (${TENSORFLOW_SOURCE_DIR})
    add_subdirectory(
        "${TENSORFLOW_SOURCE_DIR}/tensorflow/lite"
        "${CMAKE_CURRENT_BINARY_DIR}/tensorflow-lite" EXCLUDE_FROM_ALL)      
endif()

if (WASI_NN_ENABLE_GPU EQUAL 1)
# Tensorflow specific:
# * https://www.tensorflow.org/lite/guide/build_cmake#available_options_to_build_tensorflow_lite
set (TFLITE_ENABLE_GPU ON)
endif ()

set (WASI_NN_DIR ${CMAKE_CURRENT_LIST_DIR})

include_directories (${WASI_NN_DIR})
include_directories (${WASI_NN_DIR}/src)
include_directories (${WASI_NN_DIR}/src/utils)

set (
    LIBC_WASI_NN_SOURCE
    ${WASI_NN_DIR}/src/wasi_nn.c
    ${WASI_NN_DIR}/src/wasi_nn_tensorflowlite.cpp
    ${WASI_NN_DIR}/src/utils/wasi_nn_app_native.c
)

set (TENSORFLOW_LIB tensorflow-lite)
