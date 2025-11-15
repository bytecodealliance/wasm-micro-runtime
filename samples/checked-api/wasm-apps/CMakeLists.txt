# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 3.14)

project(checked_api_wasm_apps)

include(CMakePrintHelpers)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

################ wasm  ################
add_executable(fib fib.c)
set_target_properties(fib PROPERTIES SUFFIX .wasm)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fib.wasm DESTINATION .)
