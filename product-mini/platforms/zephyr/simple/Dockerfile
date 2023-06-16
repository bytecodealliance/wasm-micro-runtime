# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
FROM ubuntu:20.04

ARG UNAME=wamr
ARG DOCKER_UID=1000
ARG DOCKER_GID=1000
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asian/Shanghai

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN groupadd -g ${DOCKER_GID} ${UNAME} \
    && useradd -m -l -u ${DOCKER_UID} -g ${DOCKER_GID} -s /bin/bash ${UNAME} 
ENV PATH="/home/wamr/.local/bin:$PATH"

# Install dependencies for ESPRESSIF
# hadolint ignore=DL3008
RUN apt-get update && apt-get install -y git wget flex bison gperf python3 python3-pip python3-venv \
    cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0 --no-install-recommends \
    && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Get ESP-IDF
RUN mkdir -p ~/esp
WORKDIR /home/wamr/esp
RUN git clone --recursive https://github.com/espressif/esp-idf.git

# Set up the tools
WORKDIR /home/wamr/esp/esp-idf
RUN ./install.sh esp32 esp32c3

# Install dependencies packages for Zephyr
# hadolint ignore=DL3008
RUN apt-get update \
    && apt-get install -y git cmake ninja-build gperf \
    ccache dfu-util device-tree-compiler wget \
    python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file libpython3.8-dev \
    make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1 --no-install-recommends \
    && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# Install recent CMake version
WORKDIR /tmp
RUN mkdir /opt/cmake \
    && wget --progress=dot:giga https://github.com/Kitware/CMake/releases/download/v3.22.1/cmake-3.22.1-linux-x86_64.sh \
    && sh cmake-3.22.1-linux-x86_64.sh --skip-license --prefix=/opt/cmake && rm cmake-3.22.1-linux-x86_64.sh
ENV PATH="/opt/cmake/bin:$PATH"

# Install the Zephyr Software Development Kit (SDK)
WORKDIR /opt
RUN wget --progress=dot:giga https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.1/zephyr-sdk-0.16.1_linux-x86_64.tar.xz \
    && wget --progress=dot:giga -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.1/sha256.sum | shasum --check --ignore-missing \
    && tar xvf zephyr-sdk-0.16.1_linux-x86_64.tar.xz && rm zephyr-sdk-0.16.1_linux-x86_64.tar.xz

SHELL ["/bin/bash", "-c"]

WORKDIR /opt/zephyr-sdk-0.16.1
RUN yes | ./setup.sh

# Get Zephyr and install Python dependencies
# USER ${UNAME}
# RUN pip3 install --user -U west && west init /home/wamr/zephyrproject

# WORKDIR /home/wamr/zephyrproject
# RUN west update 

# WORKDIR /home/wamr/zephyrproject/zephyr
# RUN git checkout v2.2.0 && west zephyr-export && pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt

# # Git clone wamr
# WORKDIR /home/wamr
# RUN git clone https://github.com/bytecodealliance/wasm-micro-runtime.git

# WORKDIR /home/wamr/wasm-micro-runtime/product-mini/platforms/zephyr/simple

# TODO: . /home/wamr/esp/esp-idf/export.sh