# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

################  WASI-SDK ################
find_path(WASI_SDK_HOME
  NAMES wasi-sdk
  PATHS /opt/
  REQUIRED
)

if (NOT WASI_SDK_HOME)
  message(FATAL_ERROR
    "can not find wasi-sdk. "
    "please download it from "
    "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-12/wasi-sdk-12.0-linux.tar.gz "
    "and install it under /opt/wasi-sdk"
  )
else()
  message(STATUS 
    "Detecting wasi-sdk info: ${WASI_SDK_HOME}/wasi-sdk"
  )
endif()

#
# check clang version
execute_process(COMMAND
  ${WASI_SDK_HOME}/wasi-sdk/bin/clang --version
  OUTPUT_VARIABLE clang_full_version_string
)
string(REGEX REPLACE ".*clang version ([0-9]+\\.[0-9]+).*" "\\1"
  CLANG_VERSION_STRING ${clang_full_version_string}
)
message(STATUS "Detecting clang versoin: ${CLANG_VERSION_STRING}")
if(CLANG_VERSION_STRING VERSION_LESS 11.0)
  message(FATAL_ERROR
    "please install latest wai-sdk to get a clang-11 at least"
  )
endif()

################  EMCC ################
if(NOT DEFINED ENV{EMSDK})
  message(FATAL_ERROR
    "can not find emsdk. "
    "please refer to https://emscripten.org/docs/getting_started/downloads.html "
    "and install it, "
    "or active emsdk by 'source ./emsdk_env.sh'"
  )
endif()

message(STATUS "Detecting EMSDK info: $ENV{EMSDK}")

### check if the emsdk is 2.0.12
### upstream/.emsdk_version should be releases-upstream-dcf819a7821f8db0c8f15ac336fea8960ec204f5-64bit
file(STRINGS "$ENV{EMSDK}/upstream/.emsdk_version" EMSDK_VERSION)
if(NOT (${EMSDK_VERSION} STREQUAL "releases-upstream-dcf819a7821f8db0c8f15ac336fea8960ec204f5-64bit"))
    message(FATAL_ERROR "please install emsdk 2.0.12")
endif()

################  BINARYEN ################
find_program(WASM_OPT
    NAMES wasm-opt
    PATHS /opt/binaryen-version_101/bin /opt/binaryen/bin
)

if (NOT WASM_OPT)
  message(FATAL_ERROR
    "can not find wasm-opt. "
    "please download it from "
    "https://github.com/WebAssembly/binaryen/releases/download/version_101/binaryen-version_101-x86_64-linux.tar.gz "
    "and install it under /opt"
  )
else()
  message(STATUS 
    "Detecting EMSDK info: $ENV{EMSDK}"
  )
endif()
