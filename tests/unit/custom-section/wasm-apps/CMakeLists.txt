# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)
project(wasm-apps)

add_executable(app.wasm app.c)
target_compile_options(app.wasm PUBLIC -g -nostdlib)
target_link_options(app.wasm PRIVATE
  -nostdlib
  LINKER:--allow-undefined
  LINKER:--export-all
  LINKER:--no-entry
)

# install .wasm
set(
  WASM_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/app.wasm
)
install(FILES ${WASM_FILES} DESTINATION .)
