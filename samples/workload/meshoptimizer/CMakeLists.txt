# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 3.0)

project(bench-meshoptimizer)

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
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/../cmake/toolchain.cmake ${CMAKE_CURRENT_SOURCE_DIR}/meshoptimizer
  BUILD_COMMAND  make codecbench.opt
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy ./codecbench.opt.wasm ${CMAKE_CURRENT_SOURCE_DIR}/build/codecbench.wasm
)
