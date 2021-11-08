# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

ARG VARIANT=focal
FROM ubuntu:${VARIANT}

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asian/Shanghai

RUN apt update \
  && apt install -y apt-transport-https apt-utils build-essential \
      ca-certificates curl g++-multilib git gnupg \
      libgcc-9-dev lib32gcc-9-dev lsb-release \
      ninja-build  ocaml ocamlbuild python2.7 \
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
# install emsdk (may not necessary ?)
RUN cd /opt \
  && git clone https://github.com/emscripten-core/emsdk.git
RUN cd /opt/emsdk \
  && git pull \
  && ./emsdk install 2.0.26 \
  && ./emsdk activate 2.0.26 \
  && echo "source /opt/emsdk/emsdk_env.sh" >> /root/.bashrc

#
#install wabt
ARG WABT_VER=1.0.24
RUN wget -c https://github.com/WebAssembly/wabt/releases/download/${WABT_VER}/wabt-${WABT_VER}-ubuntu.tar.gz -P /opt
RUN tar xf /opt/wabt-${WABT_VER}-ubuntu.tar.gz -C /opt \
  && ln -fs /opt/wabt-${WABT_VER} /opt/wabt
RUN rm /opt/wabt-${WABT_VER}-ubuntu.tar.gz

#
# install binaryen
ARG BINARYEN_VER=version_101
RUN wget -c https://github.com/WebAssembly/binaryen/releases/download/${BINARYEN_VER}/binaryen-${BINARYEN_VER}-x86_64-linux.tar.gz -P /opt
RUN tar xf /opt/binaryen-${BINARYEN_VER}-x86_64-linux.tar.gz -C /opt \
  && ln -fs /opt/binaryen-${BINARYEN_VER} /opt/binaryen
RUN rm /opt/binaryen-${BINARYEN_VER}-x86_64-linux.tar.gz


#
# install bazelisk
ARG BAZELISK_VER=1.10.1
RUN mkdir /opt/bazelisk
RUN wget -c https://github.com/bazelbuild/bazelisk/releases/download/v${BAZELISK_VER}/bazelisk-linux-amd64 -P /opt/bazelisk
RUN chmod a+x /opt/bazelisk/bazelisk-linux-amd64 \
  && ln -fs /opt/bazelisk/bazelisk-linux-amd64 /opt/bazelisk/bazel

# set path
RUN echo "PATH=/opt/clang_llvm/bin:/opt/wasi-sdk/bin:/opt/wabt/bin:/opt/binaryen/bin:/opt/bazelisk:${PATH}" >> /root/.bashrc

#
# PS
RUN echo "PS1='\n[ \u@wamr-dev-docker \W ]\n$ '" >> /root/.bashrc

# Clean up
RUN apt-get autoremove -y \
  && apt-get clean -y \
  && rm -rf /var/lib/apt/lists/* \
  && rm -rf /tmp/*

VOLUME workspace
WORKDIR workspace
