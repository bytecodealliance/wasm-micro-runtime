# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 2.8...3.18)
project(network_sample)

message(CHECK_START "Detecting WASI-SDK")
if(NOT (DEFINED WASI_SDK_DIR OR DEFINED CACHE{WASI_SDK_DIR}))
  find_path(WASI_SDK_PARENT
    wasi-sdk
    PATHS /opt
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
  )
  if(WASI_SDK_PARENT)
    set(WASI_SDK_DIR ${WASI_SDK_PARENT}/wasi-sdk)
  endif()
endif()
if(WASI_SDK_DIR)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
endif()

if(NOT EXISTS ${WASI_SDK_DIR})
  message(FATAL_ERROR "Please set the absolute path of wasi-sdk with \'cmake -DWASI_SDK_HOME=XXX\'")
endif()

message(CHECK_START "Detecting WASI_TOOLCHAIN_FILE at ${WASI_SDK_DIR}")
find_file(WASI_TOOLCHAIN_FILE
  wasi-sdk.cmake
  PATHS "${WASI_SDK_DIR}/share/cmake"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
if(WASI_TOOLCHAIN_FILE)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
endif()

if(WASI_TOOLCHAIN_FILE-NOTFOUND)
  message(FATAL_ERROR "Can not find wasi-sdk.cmake under ${WASI_SDK_DIR}")
endif()

message(CHECK_START "Detecting WASI_SYS_ROOT at ${WASI_SDK_DIR}")
find_path(WASI_SYS_ROOT
  wasi-sysroot
  PATHS "${WASI_SDK_DIR}/share"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
if(WASI_SYS_ROOT)
  message(CHECK_PASS "found")
  set(WASI_SYS_ROOT ${WASI_SYS_ROOT}/wasi-sysroot)
else()
  message(CHECK_FAIL "not found")
endif()

if(WASI_SYS_ROOT-NOTFOUND)
  message(FATAL_ERROR "Can not find wasi-sysroot/ under ${WASI_SDK_DIR}")
endif()

message(STATUS "WASI_SDK_DIR is ${WASI_SDK_DIR}")
message(STATUS "WASI_TOOLCHAIN_FILE is ${WASI_TOOLCHAIN_FILE}")
message(STATUS "WASI_SYS_ROOT is ${WASI_SYS_ROOT}")

#######################################
include(ExternalProject)

ExternalProject_Add(wasm-app
  SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/wasm-src
  UPDATE_COMMAND    ""
  PATCH_COMMAND     ""
  CONFIGURE_COMMAND ${CMAKE_COMMAND}
                      -DWASI_SDK_PREFIX=${WASI_SDK_DIR}
                      -DCMAKE_TOOLCHAIN_FILE=${WASI_TOOLCHAIN_FILE}
                      -DCMAKE_SYSROOT=${WASI_SYS_ROOT}
                      ${CMAKE_CURRENT_SOURCE_DIR}/wasm-src
  BUILD_COMMAND     ${CMAKE_COMMAND} --build .
  INSTALL_COMMAND   ${CMAKE_COMMAND} -E copy
                      tcp_client.wasm ${CMAKE_CURRENT_SOURCE_DIR}/build
                      tcp_server.wasm ${CMAKE_CURRENT_SOURCE_DIR}/build
                      tcp_client.wasm.dump ${CMAKE_CURRENT_SOURCE_DIR}/build
                      tcp_server.wasm.dump ${CMAKE_CURRENT_SOURCE_DIR}/build
                    && ${CMAKE_COMMAND} -E create_symlink
                      ${CMAKE_CURRENT_SOURCE_DIR}/../../product-mini/platforms/linux/build/iwasm
                      ${CMAKE_CURRENT_SOURCE_DIR}/build/iwasm
)

add_executable(tcp_server ${CMAKE_CURRENT_SOURCE_DIR}/wasm-src/tcp_server.c)
add_executable(tcp_client ${CMAKE_CURRENT_SOURCE_DIR}/wasm-src/tcp_client.c)
