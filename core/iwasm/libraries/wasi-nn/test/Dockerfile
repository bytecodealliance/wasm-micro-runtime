# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

FROM ubuntu:20.04 as base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake build-essential git wget python3.10 python3-pip

ARG WASI_SDK_VER=16
RUN wget -c --progress=dot:giga https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VER}/wasi-sdk-${WASI_SDK_VER}.0-linux.tar.gz -P /opt \
  && tar xf /opt/wasi-sdk-${WASI_SDK_VER}.0-linux.tar.gz -C /opt \
  && ln -fs /opt/wasi-sdk-${WASI_SDK_VER}.0 /opt/wasi-sdk \
  && rm /opt/wasi-sdk-${WASI_SDK_VER}.0-linux.tar.gz

WORKDIR /home/wamr

COPY core/deps/install_tensorflow.sh core/deps/install_tensorflow.sh
RUN ./core/deps/install_tensorflow.sh

COPY core/iwasm/libraries/wasi-nn/test/requirements.txt .
RUN pip3 install -r requirements.txt

COPY core core
COPY build-scripts build-scripts
COPY product-mini product-mini

WORKDIR /home/wamr/core/iwasm/libraries/wasi-nn/test/build

# GPU delegate: https://www.tensorflow.org/lite/guide/build_cmake#available_options_to_build_tensorflow_lite
RUN cmake \
  -DWAMR_BUILD_WASI_NN=1 \
  -DTFLITE_ENABLE_GPU=ON \
  ..

RUN make -j $(grep -c ^processor /proc/cpuinfo)

WORKDIR /home/wamr/core/iwasm/libraries/wasi-nn/test

RUN ./build.sh

ENTRYPOINT [ "./build/iwasm",  "--dir=.", "test_tensorflow.wasm" ]

FROM nvidia/cuda:11.3.0-runtime-ubuntu20.04 as gpu

WORKDIR /home/wamr/core/iwasm/libraries/wasi-nn/test

COPY --from=base /home/wamr/core/iwasm/libraries/wasi-nn/test .

RUN apt-get update && apt-get install -y --no-install-recommends \
        ocl-icd-libopencl1 \
        ocl-icd-opencl-dev \
        clinfo && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /etc/OpenCL/vendors && \
    echo "libnvidia-opencl.so.1" > /etc/OpenCL/vendors/nvidia.icd

ENV NVIDIA_VISIBLE_DEVICES=all
ENV NVIDIA_DRIVER_CAPABILITIES=compute,utility

ENTRYPOINT [ "./build/iwasm",  "--dir=.", "test_tensorflow.wasm" ]
