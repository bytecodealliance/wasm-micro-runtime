# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#!/bin/bash

docker -v>/dev/null
if [ $? -ne 0 ]; then
    echo "\nDocker is not installed, please install docker firstly.\n"
    exit 1
fi

docker images>/dev/null
if [ $? -ne 0 ]; then
    echo "\nDocker service is not running, please start your docker service firstly.\n"
    exit 2
fi

echo "Prepare to clean up the docker containers..."

if test ! -z "$(docker ps -a | grep wasm-toolchain-ctr)"; then
    echo "Stopping and removing wasm-toolchain-ctr container..."
    docker stop wasm-toolchain-ctr>/dev/null
    docker rm wasm-toolchain-ctr>/dev/null
    echo "Done."
fi

if test ! -z "$(docker ps -a | grep wasm-debug-server-ctr)"; then
    echo "Stopping and removing wasm-debug-server-ctr container..."
    docker stop wasm-debug-server-ctr>/dev/null
    docker rm wasm-debug-server-ctr>/dev/null
    echo "Done."
fi

echo "Clean up docker containers successfully."