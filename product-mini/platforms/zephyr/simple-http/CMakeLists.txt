# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.8.2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wamr)

enable_language (ASM)

set (WAMR_BUILD_PLATFORM "zephyr")

# WAMR Configuration:
set (WAMR_BUILD_TARGET "THUMB")
set (WAMR_BUILD_INTERP 1)
set (WAMR_BUILD_AOT 1)
set (WAMR_BUILD_LIBC_BUILTIN 1)
set (WAMR_BUILD_LIBC_WASI 1) # 1 to use sockets 
set (WAMR_BUILD_LIB_PTHREAD 0)
set (WAMR_BUILD_GLOBAL_HEAP_POOL 1)
set (WAMR_BUILD_GLOBAL_HEAP_SIZE 131072) # 128 KB

# Environment variables:

# Check if WAMR_ROOT_DIR is set 
if(DEFINED ENV{WAMR_ROOT_DIR})
  set(WAMR_ROOT_DIR $ENV{WAMR_ROOT_DIR})
else()
  message(FATAL_ERROR "'WAMR_ROOT_DIR' need to be specified")
endif()
message("wasi-sdk was found at ${WAMR_ROOT_DIR}")

# Check if WASI_SDK_PATH is set 
if(NOT $ENV{WASI_SDK_PATH} STREQUAL "")
  set(WASI_SDK_PATH $ENV{WASI_SDK_PATH})
else()
  find_program(WASM_C_COMPILER clang /opt/wasi-sdk/bin NO_DEFAULT_PATH)
  if(NOT WASM_C_COMPILER)
    message(FATAL_ERROR "'wasi-sdk' not found, please ensure wasi-sdk is installed.\
                         You can download and install it from\
                         https://github.com/WebAssembly/wasi-sdk/releases")
  else()
    set(WASI_SDK_PATH /opt/wasi-sdk)
  endif()
endif()
message("wasi-sdk was found at ${WASI_SDK_PATH}")

# Check if WAMR_APP_FRAMEWORK_DIR is set
if (DEFINED ENV{WAMR_APP_FRAMEWORK_DIR})
  set(WAMR_APP_FRAMEWORK_DIR $ENV{WAMR_APP_FRAMEWORK_DIR})
else()
  message(FATAL_ERROR "'wamr-app-framework' not found, please ensure they are installed.\
                       You can download and install them from\
                       https://github.com/bytecodealliance/wamr-app-framework")
endif()
message("wamr-app-framework was found at ${WAMR_APP_FRAMEWORK_DIR}")

# set the WAMR_SDK_DIR with the path specified in the environment variable
set(WAMR_SDK_DIR
    ${WAMR_APP_FRAMEWORK_DIR}/wamr-sdk
)

# set the WAMR_LIBC_BUILTIN_DIR
set(WAMR_LIBC_BUILTIN_DIR
    ${WAMR_SDK_DIR}/wamr-sdk/app/libc-builtin-sysroot
)

# set the WAMR_SDK_PACKAGE_OUT_DIR 
set(WAMR_SDK_PACKAGE_OUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/wamr-sdk/app-sdk/wamr-app-framework
)

# temp
set (PYTHON_EXECUTABLE
    /usr/bin/python
)
# ~temp

# # Reset linker flags
# set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
# set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
#include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake) # in socket-api sample

# Build the WAMR runtime 
target_sources(app PRIVATE
               ${WAMR_RUNTIME_LIB_SOURCE}
               src/main.c)


# Build wasm module with WAMR SDK
add_dependencies(app wamr-sdk)

include(ExternalProject)
# include(${WAMR_ROOT_DIR}/core/iwasm/libraries/lib-socket/lib_socket_wasi.cmake)
ExternalProject_Add(wamr-sdk
  SOURCE_DIR
    ${WAMR_SDK_DIR}/app
  BINARY_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/wamr-sdk
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND} -G${CMAKE_GENERATOR} ${WAMR_SDK_DIR}/app
      -DWAMR_BUILD_SDK_PROFILE=simple
      -DCONFIG_PATH=${CMAKE_CURRENT_SOURCE_DIR}/wamr_sdk_config.cmake
      -DCMAKE_TOOLCHAIN_FILE=${WAMR_SDK_DIR}/app/wamr_toolchain.cmake
      -DOUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/wamr-sdk
      -DWASI_SDK_DIR=${WASI_SDK_PATH}
  BUILD_COMMAND
    ${CMAKE_COMMAND} --build .
  INSTALL_COMMAND
    ""
  BUILD_BYPRODUCTS
    ${WAMR_SDK_PACKAGE_OUT_DIR}/include/wasm_app.h
    ${WAMR_SDK_PACKAGE_OUT_DIR}/lib/libapp_framework.a
  USES_TERMINAL
)

# Curently only one app is present
set(WASM_APPS
    http_get
)

foreach(WASM_APP ${WASM_APPS})
  # Build a wasm module
  add_custom_target(${WASM_APP} ALL
    COMMENT
      "Building ${WASM_APP}.wasm .."
    COMMAND
      ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps
    COMMAND
      ${WASI_SDK_PATH}/bin/clang -O3
        -I${WAMR_LIBC_BUILTIN_DIR}/include
        -I${WAMR_SDK_PACKAGE_OUT_DIR}/include
        -L${WAMR_SDK_PACKAGE_OUT_DIR}/lib
        -lapp_framework
        -z stack-size=8192 -Wl,--initial-memory=65536
        -Wl,--no-entry -nostdlib -Wl,--allow-undefined
        -Wl,--export=__heap_base,--export=__data_end
        -Wl,--export=main
        -o ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${WASM_APP}.wasm
        ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/${WASM_APP}.c
    DEPENDS
      wamr-sdk
      ${CMAKE_CURRENT_SOURCE_DIR}/wasm-apps/${WASM_APP}.c
    BYPRODUCTS
      ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${WASM_APP}.wasm
    USES_TERMINAL
  )
  
  # Run python script to generate the header file 
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/${WASM_APP}.h
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/to_c_header.py
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/wasm-apps/${WASM_APP}.wasm
    COMMENT "Generating .h file from .wasm file..."
  )

  # Add the generated header file as a target
  add_custom_target(${WASM_APP}.h ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/${WASM_APP}.h)

endforeach()