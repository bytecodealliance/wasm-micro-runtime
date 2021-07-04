#!/bin/bash

# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

docker build -t wamr_dev:0.1 -f Dockerfile . \
  && docker run --rm -it \
       --name wamr_building \
       --mount type=bind,src=$(realpath .)/..,dst=/source \
       --workdir /source \
       wamr_dev:0.1 \
       /bin/bash -c "\
         pushd product-mini/platforms/linux \
           && mkdir -p build  \
           && pushd build \
           && rm -rf * \
           && cmake .. \
           && make \
           && popd \
           && popd \
           && echo 'Copying binary for image build' \
           && mkdir -p build_out \
           && rm build_out/* \
           && cp -f product-mini/platforms/linux/build/iwasm build_out/iwasm"
