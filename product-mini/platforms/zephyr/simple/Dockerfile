# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# Refer to https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html
# for more information on how to set up the Zephyr development environment.
FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asian/Shanghai
ARG ZEPHYR_SDK_VERSION=0.16.9
# In west_lite.yml, the Zephyr version is set to v3.7.0
#ARG ZEPHYR_VERSION=3.7.0

# Install dependencies for Zephyr
# hadolint ignore=DL3008
RUN apt-get update && apt-get install -y --no-install-recommends git cmake ninja-build gperf \
    ccache dfu-util device-tree-compiler wget \
    python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
    make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1 \ 
    && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Install the Zephyr Software Development Kit (SDK)
WORKDIR /opt
# hadolint ignore=DL4006
RUN wget --progress=dot:giga https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64.tar.xz \
    && wget --progress=dot:giga -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/sha256.sum | shasum --check --ignore-missing \
    && tar xf zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64.tar.xz && rm zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64.tar.xz

WORKDIR /opt/zephyr-sdk-${ZEPHYR_SDK_VERSION}
# hadolint ignore=DL4006
# Install host tools and Register Zephyr SDK CMake package
RUN ./setup.sh -h -c

# Get Zephyr 
WORKDIR /root/zephyrproject/smoke-test

# hadolint ignore=DL3013
RUN pip3 install --no-cache-dir west
COPY ./west_lite.yml ./west.yml

# init the west workspace with a minimal manifest
RUN west init -l

WORKDIR /root/zephyrproject
RUN west update --stats

WORKDIR /root/zephyrproject/modules/zephyr
RUN west zephyr-export && pip install --no-cache-dir -r ./scripts/requirements.txt

ENV ZEPHYR_BASE="/root/zephyrproject/modules/zephyr"

# Git clone wamr
WORKDIR /root/zephyrproject/modules/
RUN git clone https://github.com/bytecodealliance/wasm-micro-runtime.git wasm-micro-runtime
WORKDIR /root/zephyrproject/modules/wasm-micro-runtime/product-mini/platforms/zephyr
