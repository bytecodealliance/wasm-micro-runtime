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
OUT_DIR="${BUILD_SCRIPT_DIR}/out"

set -xe

# 1. build xnnpack wasm files
# 1.1 to decide whether needs to build xnnpack wasm files
cd ${WAMR_DIR}/samples/workload/XNNPACK
xnnpack_out_dir=xnnpack/bazel-bin
if [ -d "$xnnpack_out_dir" ]; then
  echo "XNNPACK build directory exists"
  file_count=$(ls "$xnnpack_out_dir" | grep ".wasm$" | wc -l)
  # TODO: currently should be 116 files, may changes in the future
  if [ $file_count -eq 116 ]; then
    echo "Have fully built XNNPACK benchmark wasm files before, rebuild it to update"
    need_fresh_build=false
    cmake -B build
    cmake --build build
  else
    echo "Partially build XNNPACK wasm files, start a fresh build"
    need_fresh_build=true
  fi
else
  echo "XNNPACK build directory does not exist, start a fresh build"
  need_fresh_build=true
fi

# 1.2 only build wasm files if needed
if [ "$need_fresh_build" = true ]; then
  echo "Start building xnnpack"
  rm -fr build && mkdir build
  cmake -B build
  cmake --build build
fi

# 1.3 copy files to out/
rm -rf ${OUT_DIR}
mkdir ${OUT_DIR}
cp xnnpack/bazel-bin/*.wasm out/

# 2. compile xnnpack benchmark wasm files to benchmark aot files with wamrc
# 2.1 build wamr-compiler if needed
cd ${WAMRC_DIR}
./build_llvm.sh
rm -fr build && mkdir build
cd build && cmake ..
make
# 2.2 compile all .wasm files to .aot files
WAMRC_CMD="$(pwd)/wamrc"
cd ${OUT_DIR}
# List all .wasm files in the input directory
wasm_files=$(ls *.wasm)
# Define the number of parallel processes to use
num_procs=$(($(nproc) - 2))
if [ $num_procs -lt 2 ]; then
  num_procs=1
fi
# Define function to compile .wasm files
function compile_wasm() {
  wasm_file=$1
  WAMRC_CMD=$2
  wasm_file_name=$(basename "$wasm_file" .wasm)
  if [[ $3 == "--sgx" ]]; then
    ${WAMRC_CMD} -sgx -o "${wasm_file_name}.aot" "${wasm_file_name}.wasm"
  else
    ${WAMRC_CMD} --enable-multi-thread -o "${wasm_file_name}.aot" "${wasm_file_name}.wasm"
  fi
}
# Loop through each .wasm file and compile it in parallel
i=0
for wasm_file in $wasm_files; do
  compile_wasm "$wasm_file" "$WAMRC_CMD" "$1" &
  i=$(((i + 1) % num_procs))
  if [ $i -eq 0 ]; then
    echo "i is equal to 0"
    wait
  fi
done
# Wait for all background processes to finish
wait

# 3. build iwasm with pthread and libc_emcc enable
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

# 4. run xnnpack with iwasm
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
