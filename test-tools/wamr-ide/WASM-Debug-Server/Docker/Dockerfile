# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

FROM gcc:12.2.0 AS BASE

## set work directory
WORKDIR /root/
COPY resource /root/

# hadolint ignore=DL3008
# use `apt update -o Acquire::Check-Valid-Until=false` instead
# when timezone sync error ouccr
RUN apt update  \
    && apt-get -y install make cmake --no-install-recommends

##  -clone wamr-repo and build iwasm
RUN git clone -b main --depth=1 https://github.com/bytecodealliance/wasm-micro-runtime.git \
    && mkdir -p /root/wasm-micro-runtime/product-mini/platforms/linux/build \
    && cd /root/wasm-micro-runtime/product-mini/platforms/linux/build \
    && cmake .. -DWAMR_BUILD_DEBUG_INTERP=1 \
    && make \
    && cp /root/wasm-micro-runtime/product-mini/platforms/linux/build/iwasm /root/iwasm \
    && rm -fr /root/wasm-micro-runtime

FROM ubuntu:22.04
# COPY files from BASE image
COPY --from=BASE /root/iwasm /root
COPY --from=BASE /root/debug.sh /root
COPY --from=BASE /root/run.sh /root

WORKDIR /root/