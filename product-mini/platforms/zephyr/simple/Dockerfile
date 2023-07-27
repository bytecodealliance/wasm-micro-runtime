# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asian/Shanghai

# Install dependencies for Zephyr and ESPRESSIF
# hadolint ignore=DL3008
RUN apt-get update && apt-get install -y git wget flex bison gperf python3 python3-pip python3-venv\
    python3-dev python3-setuptools python3-tk python3-wheel xz-utils file libpython3.8-dev \
    ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 device-tree-compiler \
    make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1 qemu udev --no-install-recommends \
    && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Install recent CMake version
WORKDIR /tmp
RUN mkdir /opt/cmake \
    && wget --progress=dot:giga https://github.com/Kitware/CMake/releases/download/v3.22.1/cmake-3.22.1-linux-x86_64.sh \
    && sh cmake-3.22.1-linux-x86_64.sh --skip-license --prefix=/opt/cmake && rm cmake-3.22.1-linux-x86_64.sh
ENV PATH="/opt/cmake/bin:$PATH"

# Install the Zephyr Software Development Kit (SDK)
WORKDIR /opt
# hadolint ignore=DL4006
RUN wget --progress=dot:giga https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.1/zephyr-sdk-0.16.1_linux-x86_64.tar.xz \
    && wget --progress=dot:giga -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.1/sha256.sum | shasum --check --ignore-missing \
    && tar xvf zephyr-sdk-0.16.1_linux-x86_64.tar.xz && rm zephyr-sdk-0.16.1_linux-x86_64.tar.xz

WORKDIR /opt/zephyr-sdk-0.16.1
# hadolint ignore=DL4006
RUN yes | ./setup.sh

# Get ESP-IDF
RUN ln -s /usr/bin/python3 /usr/bin/python && mkdir -p ~/esp
WORKDIR /root/esp
RUN git clone https://github.com/espressif/esp-idf.git 
WORKDIR /root/esp/esp-idf
RUN git checkout 03d4fa28694ee15ccfd5a97447575de2d1655026 \
    && git submodule update --init --recursive
# Set up the sep-idf tools
RUN ./install.sh esp32 esp32c3

# Get Zephyr 
# hadolint ignore=DL3013
RUN pip3 install --no-cache-dir west && west init -m https://github.com/zephyrproject-rtos/zephyr --mr v3.4.0 /root/zephyrproject

WORKDIR /root/zephyrproject
RUN west update 

WORKDIR /root/zephyrproject/zephyr
RUN west zephyr-export && pip install --no-cache-dir -r ~/zephyrproject/zephyr/scripts/requirements.txt

# Git clone wamr
WORKDIR /root
RUN git clone https://github.com/bytecodealliance/wasm-micro-runtime.git

WORKDIR /root/wasm-micro-runtime/product-mini/platforms/zephyr/simple

ENV ZEPHYR_BASE="/root/zephyrproject/zephyr"
