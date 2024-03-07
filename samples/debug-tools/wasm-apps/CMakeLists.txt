# Copyright (C) 2024 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

if (APPLE)
    set (HAVE_FLAG_SEARCH_PATHS_FIRST 0)
    set (CMAKE_C_LINK_FLAGS "")
    set (CMAKE_CXX_LINK_FLAGS "")
endif ()

if (NOT DEFINED WASI_SDK_DIR)
  set (WASI_SDK_DIR               "/opt/wasi-sdk")
endif ()

if (DEFINED WASI_SYSROOT)
  set (CMAKE_SYSROOT                "${WASI_SYSROOT}")
endif ()

set (CMAKE_C_COMPILER               "${WASI_SDK_DIR}/bin/clang")
set (CMAKE_ASM_COMPILER               "${WASI_SDK_DIR}/bin/clang")
set (CMAKE_EXE_LINKER_FLAGS         "-target wasm32-wasi")

################ wabt and wamrc dependencies ################
message(CHECK_START "Detecting WABT")
if(NOT (DEFINED WABT_DIR OR DEFINED CACHE{WABT_DIR}))
  find_path(WABT_DIR
    wabt
    PATHS /opt
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
  )
  if(DEFINED WABT_DIR)
    set(WABT_DIR ${WABT_DIR}/wabt)
  endif()
endif()
if(WABT_DIR)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
endif()

message(CHECK_START "Detecting WASM_OBJDUMP at ${WABT_DIR}")
find_program(WASM_OBJDUMP
  wasm-objdump
  PATHS "${WABT_DIR}/bin"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
if(WASM_OBJDUMP)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
endif()
if((NOT EXISTS ${WASM_OBJDUMP}) )
  message(FATAL_ERROR "Please make sure to have wasm-objdump under the path=${WABT_DIR}/bin ")
endif()

set(WAMR_COMPILER_DIR ${CMAKE_CURRENT_LIST_DIR}/../../wamr-compiler/build)
message(CHECK_START "Detecting WAMR_COMPILER at ${WAMR_COMPILER_DIR}")
find_file(WAMR_COMPILER
  wamrc
  PATHS "${CMAKE_CURRENT_LIST_DIR}/../../../wamr-compiler/build"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
if(WAMR_COMPILER)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
endif()
if((NOT EXISTS ${WAMR_COMPILER}) )
  message(FATAL_ERROR "Please build wamrc under the path=${WAMR_ROOT_DIR}/wamr-compiler/")
endif()

################ wasm and aot compilation ################
function (compile_sample SOURCE_FILE)
  get_filename_component (FILE_NAME ${SOURCE_FILE} NAME_WLE)
  set (WASM_MODULE ${FILE_NAME}.wasm)
  add_executable (${WASM_MODULE} ${SOURCE_FILE})

  add_custom_target(
    wasm_to_aot
    ALL
    DEPENDS ${WAMR_COMPILER} ${WASM_MODULE}
    # Use --enable-dump-call-stack to generate stack trace (addr2line)
    COMMAND ${WAMR_COMPILER} --size-level=0 --enable-dump-call-stack -o wasm-apps/trap.aot wasm-apps/trap.wasm
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endfunction ()

set(CMAKE_BUILD_TYPE Debug)  # Otherwise no debug symbols (addr2line)
compile_sample(trap.c)