# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 2.8...3.16)

project(bwa_wasm)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/preparation.cmake)

#######################################
include(ExternalProject)

################ libz ################
ExternalProject_Add(libz_src
  GIT_REPOSITORY https://github.com/madler/zlib.git
  GIT_TAG        master
  GIT_PROGRESS   ON
  GIT_SHALLOW    ON
  SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/libz
  UPDATE_COMMAND    ""
  PATCH_COMMAND     ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
)

################ bwa ################
ExternalProject_Add(bwa
  GIT_REPOSITORY https://github.com/lh3/bwa.git
  GIT_TAG        master
  GIT_PROGRESS   ON
  GIT_SHALLOW    ON
  DOWNLOAD_EXTRACT_TIMESTAMP   ON
  SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/bwa
  DEPENDS        libz_src
  UPDATE_COMMAND git clean -ffdx && git checkout -- *
                  && ${CMAKE_COMMAND} -E echo "Copying pre-installed CMakeLists.txt"
                  && ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.bwa_wasm.txt CMakeLists.txt
                  && git apply ../bwa.patch
  CONFIGURE_COMMAND  ${CMAKE_COMMAND}
                       -DWASI_SDK_PREFIX=${WASI_SDK_HOME}
                       -DCMAKE_TOOLCHAIN_FILE=${WASI_SDK_HOME}/share/cmake/wasi-sdk.cmake
                       -DCMAKE_SYSROOT=${WASI_SDK_HOME}/share/wasi-sysroot
                       ${CMAKE_CURRENT_SOURCE_DIR}/bwa
  BUILD_COMMAND  make bwa_wasm_opt
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different ./bwa.opt.wasm ${CMAKE_CURRENT_BINARY_DIR}/bwa.wasm
)

################ bwa data ################
ExternalProject_Add(bwa-kit
  PREFIX            bwa-kit
  URL               https://sourceforge.net/projects/bio-bwa/files/bwakit/bwakit-0.7.15_x64-linux.tar.bz2/download
  URL_HASH          SHA256=0a7b11971bc7916b68e9df35a364afe77cb3000df02ffb3a6fbd1aff9be5878c
  DOWNLOAD_NAME     bwakit-0.7.15_x64-linux.tar.bz2
  DOWNLOAD_NO_EXTRACT          OFF
  DOWNLOAD_EXTRACT_TIMESTAMP   ON
  UPDATE_COMMAND    ""
  PATCH_COMMAND     ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ${CMAKE_COMMAND} -E copy_if_different
                      ${CMAKE_CURRENT_BINARY_DIR}/bwa-kit/src/bwa-kit/resource-GRCh38/hs38DH-extra.fa
                      ${CMAKE_CURRENT_BINARY_DIR}/hs38DH-extra.fa
)