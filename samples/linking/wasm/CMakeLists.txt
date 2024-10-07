# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 3.14)

project(linking_samples_wasm)

add_executable(import_memory import_memory.c)
target_link_options(import_memory
  PRIVATE
    LINKER:--import-memory
    # (memory 3 5) for test
    LINKER:--initial-memory=196608
    LINKER:--max-memory=327680
    LINKER:--initial-heap=65536
)
set_target_properties(import_memory PROPERTIES SUFFIX .wasm)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/import_memory.wasm DESTINATION .)
