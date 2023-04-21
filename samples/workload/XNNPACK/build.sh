#!/bin/bash

#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

#######################################
#   build xnnpack benchmark samples   #
#######################################
BUILD_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

WAMR_DIR="${BUILD_SCRIPT_DIR}/../../.."
WAMR_PLATFORM_DIR="${WAMR_DIR}/product-mini/platforms"
WAMRC_DIR="${WAMR_DIR}/wamr-compiler"
OUT_DIR="${BUILD_SCRIPT_DIR}/build/wasm-opt"

set -xe

# 1. build wamrc and xnnpack aot files using cmake
# 1.1 to decide whether needs to start a fresh build
cd ${WAMR_DIR}/samples/workload/XNNPACK
if [ -d "$OUT_DIR" ]; then
  echo "XNNPACK build directory exists"
  file_count=$(ls "$OUT_DIR" | grep ".aot$" | wc -l)
  file_count_expected=$(egrep "\/\/:.*\.wasm" CMakeLists.txt | wc -l)
  if [ $file_count -eq $file_count_expected ]; then
    echo "Have fully built XNNPACK benchmark aot files before, skip build"
    need_fresh_build=false
  else
    echo "Partially build XNNPACK aot files, start a fresh build"
    need_fresh_build=true
  fi
else
  echo "XNNPACK build directory does not exist, start a fresh build"
  need_fresh_build=true
fi

# 1.2 only build wamrc, xnnpack wasm files and aot files if needed
num_procs=$(($(nproc) - 2))
if [ $num_procs -lt 2 ]; then
  num_procs=1
fi
if [ "$need_fresh_build" = true ]; then
  echo "Start building xnnpack"
  rm -rf xnnpack && rm -fr build && mkdir build
  if [[ $1 == '--sgx' ]]; then
    cmake -B build -DSGX=1
  else
    cmake -B build
  fi
  cmake --build build -j $num_procs
  cd build && cmake --install .
fi

# 2. build iwasm with pthread and libc_emcc enable
#    platform:
#     linux by default
#     linux-sgx if $1 equals '--sgx'
if [[ $1 == '--sgx' ]]; then
  cd ${WAMR_PLATFORM_DIR}/linux-sgx
  rm -fr build && mkdir build
  cd build && cmake .. -DWAMR_BUILD_LIBC_EMCC=1
  make
  cd ../enclave-sample
  make
else
  cd ${WAMR_PLATFORM_DIR}/linux
  rm -fr build && mkdir build
  cd build && cmake .. -DWAMR_BUILD_LIB_PTHREAD=1 -DWAMR_BUILD_LIBC_EMCC=1
  make
fi

# 3. run xnnpack with iwasm
if [[ $1 == '--sgx' ]]; then
  IWASM_CMD="${WAMR_PLATFORM_DIR}/linux-sgx/enclave-sample/iwasm"
else
  IWASM_CMD="${WAMR_PLATFORM_DIR}/linux/build/iwasm"
fi
cd ${OUT_DIR}
# List all .wasm files in the input directory
aot_files=$(ls *.aot)
# Iterate through the .wasm files and compile it to aot file
for aot_file in $aot_files; do
  echo "---> run xnnpack benchmark $aot_file with iwasm"
  ${IWASM_CMD} $aot_file
done
