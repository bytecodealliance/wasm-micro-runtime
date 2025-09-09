# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)
project(wasm-apps)

add_executable(mytest.wasm mytest.c)
target_compile_options(mytest.wasm PUBLIC -nostdlib)
target_link_options(mytest.wasm PRIVATE
  -nostdlib
  LINKER:--allow-undefined
  LINKER:--export-all
  LINKER:--initial-memory=131072
  LINKER:--no-entry
)

add_executable(hello.wasm hello.c)
target_compile_options(hello.wasm PUBLIC -nostdlib)
target_link_options(hello.wasm PRIVATE
  -nostdlib
  LINKER:--allow-undefined
  LINKER:--export-all
  LINKER:--initial-memory=131072
  LINKER:--no-entry
)

# install .wasm
set(
  WASM_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/hello.wasm
  ${CMAKE_CURRENT_BINARY_DIR}/mytest.wasm
)
install(FILES ${WASM_FILES} DESTINATION .)
