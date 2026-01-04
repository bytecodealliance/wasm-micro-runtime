# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# hadolint global ignore=DL3008,DL3009

ARG VARIANT=debian-12
FROM mcr.microsoft.com/devcontainers/cpp:${VARIANT}

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

RUN apt-get update \
  && apt-get upgrade -y

RUN apt-get install -y apt-transport-https apt-utils build-essential \
  ca-certificates ccache clang-format-14 curl file g++-multilib git gnupg \
  libgcc-12-dev lib32gcc-12-dev libzstd-dev lsb-release \
  ninja-build ocaml ocamlbuild \
  python3-venv python3-pip \
  software-properties-common tree tzdata \
  unzip valgrind vim wget zip --no-install-recommends \
  && apt-get clean -y \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /opt

ARG WASI_SDK_VER=25
RUN wget -c --progress=dot:giga https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VER}/wasi-sdk-${WASI_SDK_VER}.0-x86_64-linux.tar.gz -P /tmp \
  && tar xf /tmp/wasi-sdk-${WASI_SDK_VER}.0-x86_64-linux.tar.gz -C /opt \
  && ln -sf /opt/wasi-sdk-${WASI_SDK_VER}.0-x86_64-linux /opt/wasi-sdk

ARG WABT_VER=1.0.37
RUN wget -c --progress=dot:giga https://github.com/WebAssembly/wabt/releases/download/${WABT_VER}/wabt-${WABT_VER}-ubuntu-20.04.tar.gz -P /tmp \
  && tar xf /tmp/wabt-${WABT_VER}-ubuntu-20.04.tar.gz -C /opt \
  && ln -sf /opt/wabt-${WABT_VER} /opt/wabt

# set path

# clean up
RUN apt-get autoremove -y \
  && apt-get clean -y \
  && rm -rf /var/lib/apt/lists/* \
  && rm -rf /tmp/*
