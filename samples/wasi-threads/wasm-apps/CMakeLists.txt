# Copyright (C) 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 2.8)
project(wasm-apps)

set(WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

if (APPLE)
    set (HAVE_FLAG_SEARCH_PATHS_FIRST 0)
    set (CMAKE_C_LINK_FLAGS "")
    set (CMAKE_CXX_LINK_FLAGS "")
endif ()
set(CMAKE_SYSTEM_PROCESSOR wasm32)
set (CMAKE_SYSROOT                  ${WAMR_ROOT_DIR}/wamr-sdk/app/libc-builtin-sysroot)

if (NOT DEFINED WASI_SDK_DIR)
    set (WASI_SDK_DIR               "/opt/wasi-sdk")
endif ()

set (CMAKE_C_FLAGS                  "-pthread -Qunused-arguments")
set (CMAKE_C_FLAGS                  "${CMAKE_C_FLAGS} -z stack-size=32768 -mbulk-memory")
set (CMAKE_C_COMPILER_TARGET        "wasm32-wasi")
set (CMAKE_C_COMPILER               "clang")

set (DEFINED_SYMBOLS
"${WAMR_ROOT_DIR}/wamr-sdk/app/libc-builtin-sysroot/share/defined-symbols.txt")

set (CMAKE_EXE_LINKER_FLAGS
     "-Wl,--shared-memory,--max-memory=1048576,          \
      -Wl,--no-entry,                       \
      -Wl,--export=__heap_base,--export=__data_end      \
      -Wl,--export=__wasm_call_ctors                    \
      -Wl,--export=main -Wl,--export=__main_argc_argv   \
      -Wl,--export=wasi_thread_start \
      -Wl,--allow-undefined"
      #-Wl,--allow-undefined-file=${DEFINED_SYMBOLS}"
)

add_executable(test.wasm  main.c)
target_link_libraries(test.wasm)
