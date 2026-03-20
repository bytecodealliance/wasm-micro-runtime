# Copyright (C) 2024 Xiaomi Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)
project(wasm-apps)

set(WAMR_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)

# Find WAMRC
set(WAMRC_ROOT_DIR ${WAMR_ROOT_DIR}/wamr-compiler)
find_program(WAMRC_BIN wamrc HINTS ${WAMRC_ROOT_DIR}/build REQUIRED)

# Set architecture-specific WAMRC flags
if (WAMR_BUILD_TARGET STREQUAL "X86_32")
  set(WAMRC_SHARED_HEAP_FLAGS --opt-level=3 --bounds-checks=1 --enable-shared-heap --target=i386)
  set(WAMRC_SHARED_HEAP_CHAIN_FLAGS --opt-level=3 --bounds-checks=1 --enable-shared-chain --target=i386)
else ()
  set(WAMRC_SHARED_HEAP_FLAGS --opt-level=3 --bounds-checks=1 --enable-shared-heap)
  set(WAMRC_SHARED_HEAP_CHAIN_FLAGS --opt-level=3 --bounds-checks=1 --enable-shared-chain)
endif ()

#
# C -> Wasm
#
add_executable(test.wasm test.c)
target_compile_options(test.wasm PUBLIC -nostdlib -O0 -pthread)
target_link_options(test.wasm PRIVATE
  -nostdlib
  LINKER:--no-entry
  LINKER:--initial-memory=65536
  LINKER:--allow-undefined
  LINKER:--export-all
  -z stack-size=1024
)

add_executable(test_addr_conv.wasm test_addr_conv.c)
target_compile_options(test_addr_conv.wasm PUBLIC -nostdlib -O0 -pthread)
target_link_options(test_addr_conv.wasm PRIVATE
  -nostdlib
  LINKER:--no-entry
  LINKER:--initial-memory=65536
  LINKER:--allow-undefined
  LINKER:--export-all
  -z stack-size=1024
)

# Compile AOT files (combined target)
add_custom_target(compile_aot ALL
  COMMAND ${WAMRC_BIN} ${WAMRC_SHARED_HEAP_FLAGS} -o test.aot test.wasm
  COMMAND ${WAMRC_BIN} ${WAMRC_SHARED_HEAP_CHAIN_FLAGS} -o test_chain.aot test.wasm
  COMMAND ${WAMRC_BIN} ${WAMRC_SHARED_HEAP_FLAGS} -o test_addr_conv.aot test_addr_conv.wasm
  COMMAND ${WAMRC_BIN} ${WAMRC_SHARED_HEAP_CHAIN_FLAGS} -o test_addr_conv_chain.aot test_addr_conv.wasm
  DEPENDS test.wasm test_addr_conv.wasm
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Install WASM files
set(WASM_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/test.wasm
  ${CMAKE_CURRENT_BINARY_DIR}/test_addr_conv.wasm
)
install(FILES ${WASM_FILES} DESTINATION .)

# Install AOT files
set(AOT_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/test.aot
  ${CMAKE_CURRENT_BINARY_DIR}/test_chain.aot
  ${CMAKE_CURRENT_BINARY_DIR}/test_addr_conv.aot
  ${CMAKE_CURRENT_BINARY_DIR}/test_addr_conv_chain.aot
)
install(FILES ${AOT_FILES} DESTINATION .)
