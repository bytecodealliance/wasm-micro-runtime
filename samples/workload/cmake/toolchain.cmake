# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 3.0)

if(DEFINED _WAMR_TOOLCHAIN_CMAKE_)
  return()
else()
  set(_WAMR_TOOLCHAIN_CMAKE_ 1)
endif()

SET(CMAKE_SYSTEM_NAME Linux)

################  COMPILER  ################
find_program(CLANG_11 NAMES clang clang-11 REQUIRED)
find_program(CLANG++_11 NAMES clang++ clang++-11 REQUIRED)

if(NOT CLANG_11)
  message(FATAL_ERROR "clang not found")
else()
  message(STATUS "use ${CLANG_11} as the c compiler")
endif()

if(NOT CLANG++_11)
  message(FATAL_ERROR "clang++ not found")
else()
  message(STATUS "use ${CLANG++_11} as the c++ compiler")
endif()

set(CMAKE_C_COMPILER "${CLANG_11}" CACHE STRING "C compiler" FORCE)
set(CMAKE_C_COMPILER_ID Clang CACHE STRING "C compiler ID" FORCE)

set(CMAKE_CXX_COMPILER "${CLANG++_11}" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_CXX_COMPILER_ID Clang CACHE STRING "C++ compiler ID" FORCE)

################  WASI AS SYSROOT  ################
find_path(WASI_SYSROOT
  wasi-sysroot
  PATHS /opt/wasi-sdk-11.0/share /opt/wasi-sdk/share
  REQUIRED
)

if(NOT WASI_SYSROOT)
  message(FATAL_ERROR
    "can not find wasi sysroot. "
    "please download it from "
    "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-11/wasi-sdk-11.0-linux.tar.gz "
    "and install it under /opt"
  )
endif()

set(CMAKE_SYSROOT ${WASI_SYSROOT}/wasi-sysroot CACHE STRING "--sysroot to compiler" FORCE)

add_compile_options(
  --target=wasm32-wasi
  -msimd128
  $<IF:$<CONFIG:Debug>,-O0,-O3>
  $<$<CONFIG:Debug>:-g>
  $<$<CONFIG:Debug>:-v>
)

# need users to create their own additional include files

################  AR  ################
find_program(LLVM_AR NAMES llvm-ar llvm-ar-11 REQUIRED)

if(NOT LLVM_AR)
  message(FATAL_ERROR "llvm-ar not found")
else()
  message(STATUS "use ${LLVM_AR} as the AR")
endif()

set(CMAKE_AR "${LLVM_AR}" CACHE STRING "AR" FORCE)

################  RANLIB  ################
find_program(LLVM_RANLIB NAMES llvm-ranlib llvm-ranlib-11 REQUIRED)

if(NOT LLVM_RANLIB)
  message(FATAL_ERROR "llvm-ranlib not found")
else()
  message(STATUS "use ${LLVM_RANLIB} as the ranlib")
endif()

set(CMAKE_RANLIB "${LLVM_RANLIB}" CACHE STRING "RANLIB" FORCE)

################  LD  ################
find_program(WASM_LD NAMES wasm-ld wasm-ld-11 REQUIRED)

if(NOT WASM_LD)
  message(FATAL_ERROR "wasm-ld not found")
else()
  message(STATUS "use ${WASM_LD} as the linker")
endif()

add_link_options(
  --target=wasm32-wasi
  -fuse-ld=${WASM_LD}
  LINKER:--allow-undefined
  $<IF:$<CONFIG:Debug>,-O0,-O3>
  $<$<CONFIG:Debug>:-g>
  $<$<CONFIG:Debug>:-v>
)
