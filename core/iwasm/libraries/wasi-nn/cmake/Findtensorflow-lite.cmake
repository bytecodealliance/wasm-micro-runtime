# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception


find_library(TENSORFLOW_LITE 
     NAMES tensorflow-lite
     HINTS ${CMAKE_LIBRARY_PATH}
)
find_path(TENSORFLOW_LITE_INCLUDE_DIR
     NAMES tensorflow/lite/interpreter.h
     HINTS ${CMAKE_LIBRARY_PATH}../include
)
find_path(FLATBUFFER_INCLUDE_DIR
     NAMES flatbuffers/flatbuffers.h
     HINTS ${CMAKE_LIBRARY_PATH}../include
)
include_directories (${TENSORFLOW_LITE_INCLUDE_DIR})
include_directories (${FLATBUFFER_INCLUDE_DIR})

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

    if (WASI_NN_ENABLE_GPU EQUAL 1)
    # Tensorflow specific:
    # * https://www.tensorflow.org/lite/guide/build_cmake#available_options_to_build_tensorflow_lite
    set (TFLITE_ENABLE_GPU ON)
    endif ()
endif()

