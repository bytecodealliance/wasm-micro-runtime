#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

#!/bin/bash

if [[ ! -d build_scripts ]]; then
    mkdir build_scripts
fi

WASI_SDK_VER=11.0
WABT_VER=1.0.19
CMAKE_VER=3.16.2
BINARYEN_VER=version_97
BAZEL_VER=3.7.0

cd build_scripts
if [[ ! -f wasi-sdk-${WASI_SDK_VER}-linux.tar.gz ]]; then
  wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-11/wasi-sdk-${WASI_SDK_VER}-linux.tar.gz
fi

if [[ ! -f wabt-${WABT_VER}-ubuntu.tar.gz ]]; then
  wget https://github.com/WebAssembly/wabt/releases/download/${WABT_VER}/wabt-${WABT_VER}-ubuntu.tar.gz
fi

if [[ ! -f llvm.sh ]]; then
  wget https://apt.llvm.org/llvm.sh
fi

if [[ ! -f cmake-${CMAKE_VER}-Linux-x86_64.sh ]]; then
  wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VER}/cmake-${CMAKE_VER}-Linux-x86_64.sh
fi

if [[ ! -f binaryen-${BINARYEN_VER}-x86_64-linux.tar.gz ]]; then
  wget https://github.com/WebAssembly/binaryen/releases/download/${BINARYEN_VER}/binaryen-${BINARYEN_VER}-x86_64-linux.tar.gz
fi

if [[ ! -f bazel-${BAZEL_VER}-installer-linux-x86_64.sh ]]; then
  wget https://github.com/bazelbuild/bazel/releases/download/${BAZEL_VER}/bazel-${BAZEL_VER}-installer-linux-x86_64.sh
fi
cd -

docker build \
  --build-arg http_proxy=${http_proxy} \
  --build-arg https_proxy=${https_proxy} \
  --build-arg HTTP_PROXY=${http_proxy} \
  --build-arg HTTPS_PROXY=${https_proxy} \
  --build-arg WASI_SDK_VER=11.0 \
  --build-arg WABT_VER=${WABT_VER} \
  --build-arg CMAKE_VER=${CMAKE_VER} \
  --build-arg BINARYEN_VER=${BINARYEN_VER} \
  --build-arg BAZEL_VER=${BAZEL_VER} \
  -t clang_env:0.1 -f Dockerfile build_scripts
