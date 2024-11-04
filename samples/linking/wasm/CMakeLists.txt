# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.14)

project(linking_samples_wasm)

if(WAMR_BUILD_AOT EQUAL 1)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake)
  find_package(WAMRC REQUIRED)
endif()

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

add_executable(import_table import_table.c)
target_link_options(import_table
  PRIVATE
    LINKER:--import-table
)
set_target_properties(import_table PROPERTIES SUFFIX .wasm)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/import_table.wasm DESTINATION .)

if(WAMR_BUILD_AOT EQUAL 1)
  add_custom_target (
    import_memory_aot
    ALL
    DEPENDS ${WAMRC_BIN} import_memory
    COMMAND ${WAMRC_BIN} -o import_memory.aot import_memory.wasm
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/import_memory.aot DESTINATION .)

  add_custom_target(
    import_table_aot
    ALL
    DEPENDS ${WAMRC_BIN} import_table
    COMMAND ${WAMRC_BIN} -o import_table.aot import_table.wasm
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/import_table.aot DESTINATION .)
endif()

