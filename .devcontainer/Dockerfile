# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.195.0/containers/cpp/.devcontainer/base.Dockerfile
# [Choice] Debian / Ubuntu version (use Debian 11/9, Ubuntu 18.04/21.04 on local arm64/Apple Silicon): debian-11, debian-10, debian-9, ubuntu-21.04, ubuntu-20.04, ubuntu-18.04
ARG VARIANT=ubuntu-20.04
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asian/Shanghai

RUN apt update \
  && apt install -y apt-transport-https apt-utils build-essential \
  ca-certificates curl g++-multilib git gnupg \
  libgcc-9-dev lib32gcc-9-dev lsb-release \
  ninja-build ocaml ocamlbuild python2.7 \
  software-properties-common tree tzdata \
  unzip valgrind vim wget zip

#
# CMAKE (https://apt.kitware.com/)
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg > /dev/null \
  && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null \
  && apt update \
  && rm /usr/share/keyrings/kitware-archive-keyring.gpg \
  && apt install -y kitware-archive-keyring \
  && apt install -y cmake

#
# install emsdk
RUN cd /opt \
  && git clone https://github.com/emscripten-core/emsdk.git
RUN cd /opt/emsdk \
  && git pull \
  && ./emsdk install 2.0.26 \
  && ./emsdk activate 2.0.26 \
  && echo "source /opt/emsdk/emsdk_env.sh" >> /root/.bashrc

#
# install wasi-sdk
ARG WASI_SDK_VER=16
RUN wget -c https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VER}/wasi-sdk-${WASI_SDK_VER}.0-linux.tar.gz -P /opt
RUN tar xf /opt/wasi-sdk-${WASI_SDK_VER}.0-linux.tar.gz -C /opt \
  && ln -fs /opt/wasi-sdk-${WASI_SDK_VER}.0 /opt/wasi-sdk
RUN rm /opt/wasi-sdk-${WASI_SDK_VER}.0-linux.tar.gz

#
#install wabt
ARG WABT_VER=1.0.29
RUN wget -c https://github.com/WebAssembly/wabt/releases/download/${WABT_VER}/wabt-${WABT_VER}-ubuntu.tar.gz -P /opt
RUN tar xf /opt/wabt-${WABT_VER}-ubuntu.tar.gz -C /opt \
  && ln -fs /opt/wabt-${WABT_VER} /opt/wabt
RUN rm /opt/wabt-${WABT_VER}-ubuntu.tar.gz

#
# install bazelisk
ARG BAZELISK_VER=1.12.0
RUN mkdir /opt/bazelisk
RUN wget -c https://github.com/bazelbuild/bazelisk/releases/download/v${BAZELISK_VER}/bazelisk-linux-amd64 -P /opt/bazelisk
RUN chmod a+x /opt/bazelisk/bazelisk-linux-amd64 \
  && ln -fs /opt/bazelisk/bazelisk-linux-amd64 /opt/bazelisk/bazel

#
# install clang+llvm
RUN cd /etc/apt/apt.conf.d \
  && touch 99verfiy-peer.conf \
  && echo "Acquire { https::Verify-Peer false }" > 99verfiy-peer.conf
RUN cd /tmp \
  && wget https://apt.llvm.org/llvm.sh \
  && chmod a+x ./llvm.sh
RUN /tmp/llvm.sh 12 all
RUN ln -sf /usr/bin/clang-format-12 /usr/bin/clang-format

#
# [Optional]

#
# Install pip
RUN apt update && apt install -y --reinstall python3-venv python3-pip
RUN python3 -m pip install --upgrade pip

#
# Install required python packages
RUN pip3 install --user black nose pycparser pylint

# set path
ENV PATH "/opt/bazelisk:/opt/clang-llvm/bin:${PATH}"
RUN echo "export PATH=/opt/bazelisk:/opt/clang-llvm/bin:${PATH}" >> /root/.bashrc

#
# PS
RUN echo "PS1='\n[ \u@wamr-dev-docker \W ]\n$ '" >> /root/.bashrc

# Clean up
RUN apt-get autoremove -y \
  && apt-get clean -y \
  && rm -rf /var/lib/apt/lists/* \
  && rm -rf /tmp/*
