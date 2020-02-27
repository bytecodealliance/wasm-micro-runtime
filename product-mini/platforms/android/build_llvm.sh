#!/bin/sh

# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

DEPS_DIR=${PWD}/../../../core/deps

cd ${DEPS_DIR}
if [ ! -d "llvm" ]; then
  echo "Clone llvm to core/deps/ .."
  git clone https://github.com/llvm-mirror/llvm.git
fi

cd llvm
mkdir -p build
cd build

if [ ! -f bin/llvm-lto ]; then

  CORE_NUM=$(nproc --all)
  if [ -z "${CORE_NUM}" ]; then
    CORE_NUM=1
  fi

  echo "Build llvm with" ${CORE_NUM} "cores"

  cmake .. \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          -DCMAKE_BUILD_TYPE:STRING="Release" \
          -DLLVM_BUILD_LLVM_DYLIB:BOOL=OFF \
          -DLLVM_OPTIMIZED_TABLEGEN:BOOL=ON \
          -DLLVM_INCLUDE_EXAMPLES:BOOL=OFF \
          -DLLVM_INCLUDE_TESTS:BOOL=OFF \
          -DLLVM_INCLUDE_BENCHMARKS:BOOL=OFF \
          -DLLVM_APPEND_VC_REV:BOOL=OFF
  make -j ${CORE_NUM}

else
  echo "llvm has already been built"
fi

cd ${PWD}

