# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

FROM ubuntu:18.04

RUN apt update \
  && apt install -y apt-transport-https ca-certificates gnupg \
       software-properties-common wget lsb-release curl build-essential

#
# CMAKE (https://apt.kitware.com/)
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null \
  && apt purge --auto-remove cmake \
  && apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' \
  && apt update \
  && apt-get install -y kitware-archive-keyring \
  && rm /etc/apt/trusted.gpg.d/kitware.gpg \
  && apt-get install -y cmake
