# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 3.0)

project(bench-meshoptimizer)

################  WASI-SDK ################
find_path(WASI_SDK_HOME
  NAMES wasi-sdk
  PATHS /opt/
  REQUIRED
)

if (NOT WASI_SDK_HOME)
  message(FATAL_ERROR
    "can not find wasi-sdk. "
    "please download it from "
    "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-12/wasi-sdk-12.0-linux.tar.gz "
    "and install it under /opt/wasi-sdk"
  )
endif()

#
# check clang version
execute_process(COMMAND
  ${WASI_SDK_HOME}/wasi-sdk/bin/clang --version
  OUTPUT_VARIABLE clang_full_version_string
)
string(REGEX REPLACE ".*clang version ([0-9]+\\.[0-9]+).*" "\\1"
  CLANG_VERSION_STRING ${clang_full_version_string}
)
message("cur clang version is ${CLANG_VERSION_STRING}")
if(CLANG_VERSION_STRING VERSION_LESS 11.0)
  message(FATAL_ERROR
    "please install latest wai-sdk to get a clang-11 at least"
  )
endif()

################  BINARYEN ################
find_program(WASM_OPT
    NAMES wasm-opt
    PATHS /opt/binaryen-version_97/bin /opt/binaryen/bin
)

if (NOT WASM_OPT)
  message(FATAL_ERROR
    "can not find wasm-opt. "
    "please download it from "
    "https://github.com/WebAssembly/binaryen/releases/download/version_97/binaryen-version_97-x86_64-linux.tar.gz "
    "and install it under /opt"
  )
endif()

################  MESHOPTIMIZER  ################
include(ExternalProject)

ExternalProject_Add(codecbench
  PREFIX codecbench
  GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
  GIT_TAG        master
  GIT_SHALLOW    ON
  GIT_PROGRESS   ON
  SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/meshoptimizer
  UPDATE_COMMAND git clean -fd && git checkout -- *
                   && ${CMAKE_COMMAND} -E echo "Applying patch"
                   && git apply ${CMAKE_CURRENT_SOURCE_DIR}/codecbench.patch
  CONFIGURE_COMMAND ${CMAKE_COMMAND}
                         -DWASI_SDK_PREFIX=${WASI_SDK_HOME}/wasi-sdk
                         -DCMAKE_TOOLCHAIN_FILE=${WASI_SDK_HOME}/wasi-sdk/share/cmake/wasi-sdk.cmake
                         ${CMAKE_CURRENT_SOURCE_DIR}/meshoptimizer
  BUILD_COMMAND  make codecbench.opt
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy ./codecbench.opt.wasm ${CMAKE_CURRENT_SOURCE_DIR}/build/codecbench.wasm
)
