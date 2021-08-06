# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 2.8...3.16)

project(av1_wasm)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/preparation.cmake)

#######################################
include(ExternalProject)

################ HEADERS ################
set(EMSDK_SYSTEM_HEADERS "$ENV{EMSDK}/upstream/emscripten/system/include")
set(EMSDK_LIBC_HEADERS "${EMSDK_SYSTEM_HEADERS}/libc")
ExternalProject_Add(headers_from_emcc
   PREFIX headers
   SOURCE_DIR "${EMSDK_SYSTEM_HEADERS}"
   CONFIGURE_COMMAND ""
   BUILD_COMMAND  ""
   INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/include/pthread/sys
                     && ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/include/libc/bits
                     # copy emscripten pthread related header files
                     && ${CMAKE_COMMAND} -E copy ${EMSDK_LIBC_HEADERS}/pthread.h ${CMAKE_CURRENT_SOURCE_DIR}/include/pthread/
                     && ${CMAKE_COMMAND} -E copy ${EMSDK_LIBC_HEADERS}/signal.h ${CMAKE_CURRENT_SOURCE_DIR}/include/pthread/
                     && ${CMAKE_COMMAND} -E copy ${EMSDK_LIBC_HEADERS}/netdb.h ${CMAKE_CURRENT_SOURCE_DIR}/include/pthread/
                     && ${CMAKE_COMMAND} -E copy ${EMSDK_LIBC_HEADERS}/sys/wait.h ${CMAKE_CURRENT_SOURCE_DIR}/include/pthread/sys/
                     && ${CMAKE_COMMAND} -E copy ${EMSDK_LIBC_HEADERS}/sys/socket.h ${CMAKE_CURRENT_SOURCE_DIR}/include/pthread/sys/
                     # copy emscripten setjmp headers
                     && ${CMAKE_COMMAND} -E copy ${EMSDK_LIBC_HEADERS}/setjmp.h ${CMAKE_CURRENT_SOURCE_DIR}/include/libc/setjmp.h
                     && ${CMAKE_COMMAND} -E copy ${EMSDK_SYSTEM_HEADERS}/../lib/libc/musl/arch/emscripten/bits/setjmp.h ${CMAKE_CURRENT_SOURCE_DIR}/include/libc/bits/setjmp.h
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
  CONFIGURE_COMMAND  ${CMAKE_COMMAND}
                         -DWASI_SDK_PREFIX=${WASI_SDK_HOME}/wasi-sdk
                         -DCMAKE_TOOLCHAIN_FILE=${WASI_SDK_HOME}/wasi-sdk/share/cmake/wasi-sdk.cmake
                         ${CMAKE_CURRENT_SOURCE_DIR}/av1
  BUILD_COMMAND     make testavx_opt
  INSTALL_COMMAND   ${CMAKE_COMMAND} -E copy testavx.opt.wasm ${CMAKE_CURRENT_BINARY_DIR}/testavx.wasm
)
