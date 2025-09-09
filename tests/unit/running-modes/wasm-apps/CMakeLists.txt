# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)
project(wasm-apps)

# set(CMAKE_C_FLAGS "-nostdlib -pthread -Qunused-arguments")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -z stack-size=8192 -nostdlib")

add_executable(mytest.wasm mytest.c)
target_link_libraries(mytest.wasm)
target_compile_options(mytest.wasm PRIVATE
  -pthread
)
target_link_options(mytest.wasm PRIVATE
  LINKER:--allow-undefined
  LINKER:--export-all
  LINKER:--no-entry
)

add_executable(hello.wasm hello.c)
target_link_libraries(hello.wasm)

add_custom_command(TARGET hello.wasm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/hello.wasm
        ${CMAKE_CURRENT_BINARY_DIR}/../
        COMMENT "Copy hello.wasm to the same directory of google test"
        )

add_custom_command(TARGET mytest.wasm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/mytest.wasm
        ${CMAKE_CURRENT_BINARY_DIR}/../
        COMMENT "Copy mytest.wasm to the same directory of google test"
        )
