# Copyright (C) 2024 Xiaomi Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)
project(wasm-apps)

set(WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(WAMRC_ROOT_DIR ${WAMR_ROOT_DIR}/wamr-compiler/build)

set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(CMAKE_SYSROOT ${WAMR_ROOT_DIR}/wamr-sdk/app/libc-builtin-sysroot)

if (NOT DEFINED WASI_SDK_DIR)
    set(WASI_SDK_DIR "/opt/wasi-sdk")
endif ()

set(CMAKE_C_FLAGS "-nostdlib -pthread -Qunused-arguments")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -z stack-size=8192 -nostdlib -O0")
set(CMAKE_C_COMPILER_TARGET "wasm32")
set(CMAKE_C_COMPILER "${WASI_SDK_DIR}/bin/clang")

set(DEFINED_SYMBOLS
        "${WAMR_ROOT_DIR}/wamr-sdk/app/libc-builtin-sysroot/share/defined-symbols.txt")

set(CMAKE_EXE_LINKER_FLAGS
        "-Wl,--no-entry           \
      -Wl,--initial-memory=65536  \
      -Wl,--export-all            \
      -Wl,--allow-undefined"
        )

add_executable(test.wasm test.c)
target_link_libraries(test.wasm)

add_custom_command(TARGET test.wasm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/test.wasm
        ${CMAKE_CURRENT_BINARY_DIR}/../
        COMMENT "Copy test.wasm to the same directory of google test"
        )

add_custom_command(TARGET test.wasm POST_BUILD
        COMMAND ${WAMRC_ROOT_DIR}/wamrc --opt-level=0 --enable-shared-heap --bounds-checks=1
        -o
        test.aot
        test.wasm
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/test.aot
        ${CMAKE_CURRENT_BINARY_DIR}/../
        COMMENT "Copy test.aot to the same directory of google test"
        )

add_executable(test_addr_conv.wasm test_addr_conv.c)
target_link_libraries(test.wasm)

add_custom_command(TARGET test_addr_conv.wasm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/test_addr_conv.wasm
        ${CMAKE_CURRENT_BINARY_DIR}/../
        COMMENT "Copy test_addr_conv.wasm to the same directory of google test"
        )

add_custom_command(TARGET test_addr_conv.wasm POST_BUILD
        COMMAND ${WAMRC_ROOT_DIR}/wamrc --opt-level=0 --enable-shared-heap --bounds-checks=1
        -o
        test_addr_conv.aot
        test_addr_conv.wasm
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/test_addr_conv.aot
        ${CMAKE_CURRENT_BINARY_DIR}/../
        COMMENT "Copy test_addr_conv.aot to the same directory of google test"
        )
