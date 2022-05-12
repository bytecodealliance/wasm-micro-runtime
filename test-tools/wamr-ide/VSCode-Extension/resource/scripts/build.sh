# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#!/bin/bash

AoT_Binary_Name=$1

# start & run docker image and build wasm
if test ! -z "$(docker ps -a | grep wasm-toolchain-ctr)"; then
    docker rm wasm-toolchain-ctr
fi

docker run --name=wasm-toolchain-ctr \
                -it -v $(pwd):/mnt \
                wasm-toolchain:1.0  \
                /bin/bash -c "./build_wasm.sh $AoT_Binary_Name"

# stop and remove wasm-toolchain-ctr container
docker stop wasm-toolchain-ctr && docker rm wasm-toolchain-ctr
