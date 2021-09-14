#!/usr/bin/env bash
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

readonly BUILD_CONTENT="/tmp/build_content"
if [[ ! -d ${BUILD_CONTENT} ]]; then
  mkdir ${BUILD_CONTENT}
fi

readonly WASI_SDK_VER=12
readonly WABT_VER=1.0.23
readonly CMAKE_VER=3.16.2
readonly BINARYEN_VER=version_101
readonly BAZELISK_VER=1.7.5

cd ${BUILD_CONTENT} || exit
if [[ ! -f wasi-sdk-${WASI_SDK_VER}.0-linux.tar.gz ]]; then
  wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VER}/wasi-sdk-${WASI_SDK_VER}.0-linux.tar.gz
fi

if [[ ! -f wabt-${WABT_VER}-ubuntu.tar.gz ]]; then
  wget https://github.com/WebAssembly/wabt/releases/download/${WABT_VER}/wabt-${WABT_VER}-ubuntu.tar.gz
fi

if [[ ! -f cmake-${CMAKE_VER}-Linux-x86_64.sh ]]; then
  wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VER}/cmake-${CMAKE_VER}-Linux-x86_64.sh
fi

if [[ ! -f binaryen-${BINARYEN_VER}-x86_64-linux.tar.gz ]]; then
  wget https://github.com/WebAssembly/binaryen/releases/download/${BINARYEN_VER}/binaryen-${BINARYEN_VER}-x86_64-linux.tar.gz
fi

if [[ ! -f bazelisk-linux-amd64 ]]; then
  wget https://github.com/bazelbuild/bazelisk/releases/download/v${BAZELISK_VER}/bazelisk-linux-amd64
fi
cd - > /dev/null || exit

DOCKERFILE_PATH=$(dirname "$(realpath "$0")")

docker build \
    --build-arg WASI_SDK_VER=${WASI_SDK_VER} \
    --build-arg WABT_VER=${WABT_VER} \
    --build-arg CMAKE_VER=${CMAKE_VER} \
    --build-arg BINARYEN_VER=${BINARYEN_VER} \
    -t wamr_workload_env:0.1 -f "${DOCKERFILE_PATH}"/Dockerfile ${BUILD_CONTENT} \
  && docker run --rm \
      --name workload_w_clang \
      --mount type=bind,source="$(pwd)",target=/data/project \
      -w /data/project \
      wamr_workload_env:0.1 \
      /bin/bash -c /build.sh
