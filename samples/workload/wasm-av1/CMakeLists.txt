# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 3.0)

project(av1_wasm)

################  BINARYEN ################
find_program(WASM_OPT
    NAMES wasm-opt
    PATHS /opt/binaryen-version_97/bin /opt/binaryen/bin
)

if (NOT WASM_OPT)
  message(FATAL_ERROR
    "can not find wasm-opt. "
    "please download it from "
    "https://github.com/WebAssembly/binaryen/releases/download/version_97/binaryen-version_97-x86_64-linux.tar.gz "
    "and install it under /opt"
  )
endif()

#######################################
include(ExternalProject)

################ HEADERS ################
ExternalProject_Add(headers_from_emcc
   PREFIX headers
   SOURCE_DIR "$ENV{EMSDK}/upstream/emscripten/system/"
   CONFIGURE_COMMAND ""
   BUILD_COMMAND  ""
   INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/include/pthread/sys
                     && ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/include/libc/bits
                     # copy emscripten pthread related header files
                     && ${CMAKE_COMMAND} -E copy $ENV{EMSDK}/upstream/emscripten/system/include/libc/pthread.h ${CMAKE_CURRENT_SOURCE_DIR}/include/pthread/
                     && ${CMAKE_COMMAND} -E copy $ENV{EMSDK}/upstream/emscripten/system/include/libc/signal.h ${CMAKE_CURRENT_SOURCE_DIR}/include/pthread/
                     && ${CMAKE_COMMAND} -E copy $ENV{EMSDK}/upstream/emscripten/system/include/libc/netdb.h ${CMAKE_CURRENT_SOURCE_DIR}/include/pthread/
                     && ${CMAKE_COMMAND} -E copy $ENV{EMSDK}/upstream/emscripten/system/include/libc/sys/wait.h ${CMAKE_CURRENT_SOURCE_DIR}/include/pthread/sys/
                     && ${CMAKE_COMMAND} -E copy $ENV{EMSDK}/upstream/emscripten/system/include/libc/sys/socket.h ${CMAKE_CURRENT_SOURCE_DIR}/include/pthread/sys/
                     # copy emscripten setjmp headers
                     && ${CMAKE_COMMAND} -E copy $ENV{EMSDK}/upstream/emscripten/system/include/libc/setjmp.h ${CMAKE_CURRENT_SOURCE_DIR}/include/libc/setjmp.h
                     && ${CMAKE_COMMAND} -E copy $ENV{EMSDK}/upstream/emscripten/system/lib/libc/musl/arch/emscripten/bits/setjmp.h ${CMAKE_CURRENT_SOURCE_DIR}/include/libc/bits/setjmp.h
)

################ av1 ################
ExternalProject_Add(av1
  PREFIX av1
  GIT_REPOSITORY https://github.com/GoogleChromeLabs/wasm-av1.git
  GIT_TAG        master
  GIT_PROGRESS   ON
  GIT_SHALLOW    ON
  SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/av1
  DEPENDS        headers_from_emcc
  UPDATE_COMMAND git clean -fd && git checkout -- *
                  && ${CMAKE_COMMAND} -E echo "Copying pre-installed CMakeLists.txt"
                  && ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.avx_wasm.txt CMakeLists.txt
                  && git apply ../av1-clang.patch
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/../cmake/toolchain.cmake ${CMAKE_CURRENT_SOURCE_DIR}/av1
  BUILD_COMMAND     make testavx_opt
  INSTALL_COMMAND   ${CMAKE_COMMAND} -E copy testavx.opt.wasm ${CMAKE_CURRENT_BINARY_DIR}/testavx.wasm
)
