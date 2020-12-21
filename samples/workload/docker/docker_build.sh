#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

#!/bin/bash

BUILD_CONTENT="/tmp/build_content"

if [[ ! -d ${BUILD_CONTENT} ]]; then
  mkdir ${BUILD_CONTENT}
fi

WASI_SDK_VER=11.0
WABT_VER=1.0.19
CMAKE_VER=3.16.2
BINARYEN_VER=version_97
BAZEL_VER=3.7.0

cd ${BUILD_CONTENT}
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
cd - > /dev/null

DOCKERFILE_PATH=$(dirname $(realpath $0))

docker build \
  --build-arg http_proxy=${http_proxy} \
  --build-arg https_proxy=${https_proxy} \
  --build-arg HTTP_PROXY=${http_proxy} \
  --build-arg HTTPS_PROXY=${https_proxy} \
  --build-arg WASI_SDK_VER=${WASI_SDK_VER} \
  --build-arg WABT_VER=${WABT_VER} \
  --build-arg CMAKE_VER=${CMAKE_VER} \
  --build-arg BINARYEN_VER=${BINARYEN_VER} \
  --build-arg BAZEL_VER=${BAZEL_VER} \
  -t clang_env:0.1 -f ${DOCKERFILE_PATH}/Dockerfile ${BUILD_CONTENT}

docker run --rm -it \
  -e http_proxy=${http_proxy} \
  -e https_proxy=${https_proxy} \
  -e HTTP_PROXY=${http_proxy} \
  -e HTTPS_PROXY=${htpps_proxy} \
  --name workload_w_clang \
  --mount type=bind,source=$(pwd),target=/data/project \
  --mount type=bind,source=$(pwd)/../cmake,target=/data/cmake \
  -w /data/project \
  clang_env:0.1 \
  /bin/bash -c /build.sh
